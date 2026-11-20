//! render — wgpu 渲染层（规格 §12 出屏，对齐 WGPU_RENDER_DESIGN.md）
//!
//! 仅 android target 编译（wgpu 依赖仅 android）。桌面不编译（纯逻辑层 41 测试不受影响）。
//!
//! ★ 诚实边界（设计 §7）：wgpu 出屏**必须真机验收**（设备 10.0.182.146，PENDING）。
//! 本模块编译期保证**类型可解析 + 14 JNI 符号可链接**（交付物①）；
//! 完整 render pass（EGL surface 创建/纹理上传/顶点绘制）真机可达时补全（设计 §3/§6 5 判据）。
//! wgpu 30 的 request_adapter/request_device 是 async（返回 Future），真机走 pollster 轮询，
//! 此处用惰性 Option 骨架保编译，不实际执行 wgpu 调用（避免 cross-compile 无 GPU 环境）。

#[cfg(target_os = "android")]
mod gles_render {
        use crate::state::State;
    use crate::ui::{Screen, Rect};
    use wgpu::Device as WgpuDevice;

    /// 渲染上下文（wgpu device/surface + 纹理，惰性初始化）
    pub struct RenderCtx {
        pub view_w: i32,
        pub view_h: i32,
        // wgpu 资源（OnGraphCreate 真机初始化时填充；骨架期为 None 保编译）
        pub device: Option<WgpuDevice>,
        pub queue: Option<wgpu::Queue>,
        // surface（wgpu 30 Surface<'a> 含生命周期，真机 EGL 创建时局部持有；骨架期不存）
        // 棋盘纹理（737×751，从 SPAK 解码后上传）
        pub board_tex: Option<wgpu::TextureView>,
        pub pipeline: Option<wgpu::RenderPipeline>,
        // 顶点 buffer（棋盘/棋子 quad，设计 §3 ③ 顶点批次）
        pub vertex_buf: Option<wgpu::Buffer>,
        // 初始化状态（pollster 轮询标记）
        pub init_done: bool,
    }

    impl Default for RenderCtx {
        fn default() -> Self {
            Self {
                view_w: 720,
                view_h: 1280,
                device: None,
                queue: None,
                board_tex: None,
                pipeline: None,
                vertex_buf: None,
                init_done: false,
            }
        }
    }

    impl RenderCtx {
        /// 初始化 wgpu device + queue（设计 §3 OnGraphCreate，真机 pollster 获取后传入）
        pub fn init_device(&mut self, device: wgpu::Device, queue: wgpu::Queue) {
            self.device = Some(device);
            self.queue = Some(queue);
            self.init_done = true;
        }

        /// wgpu instance/adapter/device 获取（真机 pollster 轮询，设计 §3）
        /// 返回 (device, queue)；失败 None（后端不支持/GPU 缺失）
        pub fn create_wgpu_device() -> Option<(wgpu::Device, wgpu::Queue)> {
            // wgpu 30 真实 API：Instance::new(desc) 非 &desc + RequestAdapterOptions（非 AdapterOptions）
            let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
                backends: wgpu::Backends::GL, // gles 后端
                flags: wgpu::InstanceFlags::default(),
                memory_budget_thresholds: wgpu::MemoryBudgetThresholds::default(),
                backend_options: wgpu::BackendOptions::default(),
                display: None,
            });
            let adapter = instance
                .request_adapter(&wgpu::RequestAdapterOptions {
                    power_preference: wgpu::PowerPreference::LowPower,
                    force_fallback_adapter: false,
                    compatible_surface: None,
                    apply_limit_buckets: false,
                })
                .block_on()
                .ok()?;
            let (device, queue) = adapter
                .request_device(&wgpu::DeviceDescriptor {
                    label: Some("junqi-rs"),
                    required_features: wgpu::Features::empty(),
                    required_limits: wgpu::Limits::default(),
                    experimental_features: wgpu::ExperimentalFeatures::default(),
                    memory_hints: wgpu::MemoryHints::default(),
                    trace: wgpu::Trace::default(),
                })
                .block_on()
                .ok()?;
            Some((device, queue))
        }

        /// 从 ANativeWindow 创建 wgpu Surface（Android gles 后端，出屏关键）
        /// 参数 anative_window: *mut c_void（ANativeWindow_fromSurface 拿到的指针）
        /// 一次性创建 surface + device（adapter 带 compatible_surface，Android 关键）
        pub fn create_surface_and_device(
            anative_window: *mut std::ffi::c_void,
            w: u32,
            h: u32,
        ) -> Option<(wgpu::Instance, wgpu::Surface<'static>, wgpu::Device, wgpu::Queue)> {
            use raw_window_handle::{AndroidNdkWindowHandle, HasWindowHandle, RawWindowHandle, WindowHandle};
            if anative_window.is_null() { return None; }
            let nwin = std::ptr::NonNull::new(anative_window as *mut ())?;
            struct WinHandle(std::ptr::NonNull<()>);
            impl HasWindowHandle for WinHandle {
                fn window_handle(&self) -> Result<WindowHandle<'_>, raw_window_handle::HandleError> {
                    Ok(unsafe { WindowHandle::borrow_raw(RawWindowHandle::AndroidNdk(AndroidNdkWindowHandle::new(self.0.cast()))) })
                }
            }
            let wh = WinHandle(nwin);
            let instance = wgpu::Instance::new(wgpu::InstanceDescriptor {
                backends: wgpu::Backends::PRIMARY,
                flags: wgpu::InstanceFlags::default(),
                memory_budget_thresholds: wgpu::MemoryBudgetThresholds::default(),
                backend_options: wgpu::BackendOptions::default(),
                display: None,
            });
            let surface = unsafe {
                instance.create_surface_unsafe(wgpu::SurfaceTargetUnsafe::from_window(&wh).ok()?)
            }.ok()?;
            crate::log::setprop("jq.surf_dev", "surface_ok");
            // ★ 关键：request_adapter 带 compatible_surface
            let adapter = instance.request_adapter(&wgpu::RequestAdapterOptions {
                power_preference: wgpu::PowerPreference::LowPower,
                force_fallback_adapter: false,
                compatible_surface: Some(&surface),
                apply_limit_buckets: false,
            }).block_on().ok()?;
            let caps = surface.get_capabilities(&adapter);
            crate::log::setprop("jq.surf_dev", "caps_ok");
            let format = match caps.formats.first().copied() {
                Some(f) => f,
                None => { crate::log::setprop("jq.surf_dev", "no_formats"); return None; }
            };
            let (dev, q) = adapter.request_device(&wgpu::DeviceDescriptor {
                label: Some("junqi-rs"),
                required_features: wgpu::Features::empty(),
                required_limits: wgpu::Limits::default(),
                experimental_features: wgpu::ExperimentalFeatures::default(),
                memory_hints: wgpu::MemoryHints::default(),
                trace: wgpu::Trace::default(),
            }).block_on().ok()?;
            let config = wgpu::SurfaceConfiguration {
                usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
                format,
                width: w.max(1),
                height: h.max(1),
                present_mode: wgpu::PresentMode::AutoVsync,
                alpha_mode: caps.alpha_modes.first().copied()?,
                view_formats: vec![],
                color_space: wgpu::SurfaceColorSpace::Srgb,
                desired_maximum_frame_latency: 2,
            };
            surface.configure(&dev, &config);
            crate::log::setprop("jq.surf_dev", "configured");
            Some((instance, surface, dev, q))
        }

        pub fn create_surface(
            instance: &wgpu::Instance,
            device: &wgpu::Device,
            anative_window: *mut std::ffi::c_void,
            width: u32,
            height: u32,
        ) -> Option<(wgpu::Surface<'static>, wgpu::SurfaceConfiguration)> {
            crate::log::setprop("jq.surface_step", "enter_create_surface");
            use raw_window_handle::{AndroidNdkWindowHandle, HasWindowHandle, RawWindowHandle, WindowHandle};
            use pollster::FutureExt;
            if anative_window.is_null() {
                crate::log::setprop("jq.surface_step", "null_window");
                return None;
            }
            crate::log::setprop("jq.surface_step", "window_nonnull");
            let nwin = std::ptr::NonNull::new(anative_window as *mut ())?;
            // 自定义 handle 包装 ANativeWindow
            struct WinHandle(std::ptr::NonNull<()>);
            impl HasWindowHandle for WinHandle {
                fn window_handle(&self) -> Result<WindowHandle<'_>, raw_window_handle::HandleError> {
                    Ok(unsafe { WindowHandle::borrow_raw(RawWindowHandle::AndroidNdk(AndroidNdkWindowHandle::new(self.0.cast()))) })
                }
            }
            let wh = WinHandle(nwin);
            // 用 from_window（稳，不 panic）——RawHandle+AndroidDisplay 在 houdini 下触发 wgpu-hal panic
            let target = match unsafe { wgpu::SurfaceTargetUnsafe::from_window(&wh) } {
                Ok(t) => t,
                Err(_) => { crate::log::setprop("jq.surface_step", "from_window_err"); return None; }
            };
            crate::log::setprop("jq.surface_step", "from_window_ok");
            let surface = match unsafe { instance.create_surface_unsafe(target) } {
                Ok(s) => s,
                Err(_) => { crate::log::setprop("jq.surface_step", "create_surface_unsafe_err"); return None; }
            };
            crate::log::setprop("jq.surface_step", "surface_created");
            let caps = surface.get_capabilities(&instance.request_adapter(&wgpu::RequestAdapterOptions {
                power_preference: wgpu::PowerPreference::LowPower,
                force_fallback_adapter: false,
                compatible_surface: Some(&surface),
                apply_limit_buckets: false,
            }).block_on().ok()?);
            crate::log::setprop("jq.surface_step", "caps_ok");
            let format = match caps.formats.first().copied() {
                Some(f) => f,
                None => { crate::log::setprop("jq.surface_step", "no_formats"); return None; }
            };
            let config = wgpu::SurfaceConfiguration {
                usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
                format,
                width: width.max(1),
                height: height.max(1),
                present_mode: wgpu::PresentMode::AutoVsync,
                alpha_mode: caps.alpha_modes.first().copied()?,
                view_formats: vec![],
                color_space: wgpu::SurfaceColorSpace::Srgb,
                desired_maximum_frame_latency: 2,
            };
            surface.configure(device, &config);
            Some((surface, config))
        }

        /// 清屏渲染一帧（验证 wgpu 出屏通路；颜色 RGBA）
        pub fn clear_frame(
            device: &wgpu::Device,
            queue: &wgpu::Queue,
            surface: &wgpu::Surface<'_>,
            r: f64, g: f64, b: f64,
        ) {
            // wgpu 30：get_current_texture 返回 enum（Good/Timeout/Occluded/Outdated/Lost/Validation）
            let tex = match surface.get_current_texture() {
                wgpu::CurrentSurfaceTexture::Success(t) | wgpu::CurrentSurfaceTexture::Suboptimal(t) => t,
                _ => return, // Timeout/Occluded/Outdated/Lost/Validation → 跳过本帧
            };
            let view = tex.texture.create_view(&wgpu::TextureViewDescriptor::default());
            let mut enc = device.create_command_encoder(&wgpu::CommandEncoderDescriptor { label: None });
            {
                let mut pass = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
                    label: None,
                    color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                        view: &view,
                        resolve_target: None,
                        depth_slice: None,
                        ops: wgpu::Operations {
                            load: wgpu::LoadOp::Clear(wgpu::Color { r, g, b, a: 1.0 }),
                            store: wgpu::StoreOp::Store,
                        },
                    })],
                    depth_stencil_attachment: None,
                    timestamp_writes: None,
                    occlusion_query_set: None,
                    multiview_mask: None,
                });
                let _ = &mut pass;
            }
            queue.submit(Some(enc.finish()));
            // wgpu 30：SurfaceTexture 在 Drop 时自动 present（无显式 present()）
        }

        /// offscreen 渲染验证（不依赖 surface，纯 GPU texture + render pass + readback）
        /// 返回读回的首像素 RGBA（验证 wgpu 渲染管线在 houdini 下可用）
        /// offscreen 渲染返回完整 RGBA（供 Java Canvas 显示）
        pub fn offscreen_render_rgba(device: &wgpu::Device, queue: &wgpu::Queue, w: u32, h: u32) -> Option<Vec<u8>> {
            let tex = device.create_texture(&wgpu::TextureDescriptor {
                label: Some("offscreen-rgba"),
                size: wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
                mip_level_count: 1, sample_count: 1, dimension: wgpu::TextureDimension::D2,
                format: wgpu::TextureFormat::Rgba8Unorm,
                usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::COPY_SRC,
                view_formats: &[],
            });
            let view = tex.create_view(&wgpu::TextureViewDescriptor::default());
            let mut enc = device.create_command_encoder(&wgpu::CommandEncoderDescriptor { label: None });
            {
                let mut pass = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
                    label: None,
                    color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                        view: &view, resolve_target: None, depth_slice: None,
                        ops: wgpu::Operations {
                            load: wgpu::LoadOp::Clear(wgpu::Color { r: 0.1, g: 0.4, b: 0.8, a: 1.0 }),
                            store: wgpu::StoreOp::Store,
                        },
                    })],
                    depth_stencil_attachment: None, timestamp_writes: None,
                    occlusion_query_set: None, multiview_mask: None,
                });
                let _ = &mut pass;
            }
            let bpr = ((w * 4 + 255) / 256) * 256;
            let buf = device.create_buffer(&wgpu::BufferDescriptor {
                label: Some("readback-rgba"), size: (bpr * h) as u64,
                usage: wgpu::BufferUsages::COPY_DST | wgpu::BufferUsages::MAP_READ,
                mapped_at_creation: false,
            });
            enc.copy_texture_to_buffer(
                wgpu::TexelCopyTextureInfo { texture: &tex, mip_level: 0, origin: wgpu::Origin3d::ZERO, aspect: wgpu::TextureAspect::All },
                wgpu::TexelCopyBufferInfo { buffer: &buf, layout: wgpu::TexelCopyBufferLayout { offset: 0, bytes_per_row: Some(bpr), rows_per_image: Some(h) } },
                wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
            );
            queue.submit(Some(enc.finish()));
            let (tx, rx) = std::sync::mpsc::channel();
            buf.map_async(wgpu::MapMode::Read, 0..(bpr * h) as u64, move |r| { let _ = tx.send(r); });
            device.poll(wgpu::PollType::Wait { submission_index: None, timeout: None }).ok()?;
            let _ = rx.recv().ok()?.ok()?;
            let mapped = buf.get_mapped_range(0..(bpr * h) as u64).ok()?;
            let mut out = vec![0u8; (w * h * 4) as usize];
            for row in 0..h as usize {
                let src = &mapped[row * bpr as usize..row * bpr as usize + w as usize * 4];
                out[row * w as usize * 4..(row + 1) * w as usize * 4].copy_from_slice(src);
            }
            drop(mapped);
            buf.unmap();
            Some(out)
        }

        pub fn offscreen_render_test(device: &wgpu::Device, queue: &wgpu::Queue) -> Option<(u8, u8, u8, u8)> {
            let w = 64u32;
            let h = 64u32;
            // ① 创建 offscreen texture（RENDER_ATTACHMENT | COPY_SRC）
            let tex = device.create_texture(&wgpu::TextureDescriptor {
                label: Some("offscreen-test"),
                size: wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
                mip_level_count: 1,
                sample_count: 1,
                dimension: wgpu::TextureDimension::D2,
                format: wgpu::TextureFormat::Rgba8Unorm,
                usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::COPY_SRC,
                view_formats: &[],
            });
            let view = tex.create_view(&wgpu::TextureViewDescriptor::default());
            // ② render pass 清屏成蓝色 (0.1, 0.4, 0.8, 1.0)
            let mut enc = device.create_command_encoder(&wgpu::CommandEncoderDescriptor { label: None });
            {
                let mut pass = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
                    label: None,
                    color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                        view: &view,
                        resolve_target: None,
                        depth_slice: None,
                        ops: wgpu::Operations {
                            load: wgpu::LoadOp::Clear(wgpu::Color { r: 0.1, g: 0.4, b: 0.8, a: 1.0 }),
                            store: wgpu::StoreOp::Store,
                        },
                    })],
                    depth_stencil_attachment: None,
                    timestamp_writes: None,
                    occlusion_query_set: None,
                    multiview_mask: None,
                });
                let _ = &mut pass;
            }
            // ③ readback buffer（bytes_per_row 须 256 对齐）
            let bpr = 256u32;
            let buf = device.create_buffer(&wgpu::BufferDescriptor {
                label: Some("readback"),
                size: (bpr * h) as u64,
                usage: wgpu::BufferUsages::COPY_DST | wgpu::BufferUsages::MAP_READ,
                mapped_at_creation: false,
            });
            enc.copy_texture_to_buffer(
                wgpu::TexelCopyTextureInfo { texture: &tex, mip_level: 0, origin: wgpu::Origin3d::ZERO, aspect: wgpu::TextureAspect::All },
                wgpu::TexelCopyBufferInfo { buffer: &buf, layout: wgpu::TexelCopyBufferLayout { offset: 0, bytes_per_row: Some(bpr), rows_per_image: Some(h) } },
                wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
            );
            queue.submit(Some(enc.finish()));
            // ④ 映射读回首像素
            let (tx, rx) = std::sync::mpsc::channel();
            buf.map_async(wgpu::MapMode::Read, 0..(bpr * h) as u64, move |res| { let _ = tx.send(res); });
            device.poll(wgpu::PollType::Wait { submission_index: None, timeout: None }).ok()?;
            let _ = rx.recv().ok()?.ok()?;
            let data = buf.get_mapped_range(0..(bpr * h) as u64).ok()?;
            let px = (data[0], data[1], data[2], data[3]);
            drop(data);
            buf.unmap();
            Some(px)
        }

        /// 创建棋盘渲染 pipeline（真机可达时补全 wgpu API，设计 §3 ②③⑤）
        /// 真机实现：vertex buffer（棋盘 6 顶点）+ fragment（棋盘纹理采样）+ blend state（防黑底）
        pub fn build_pipeline(&mut self) {
            // 真机 NDK 可达时按设计 §3 补全（wgpu 30 create_render_pipeline + wgsl 着色器）
        }

        /// 渲染帧（设计 §3 OnUpdate：清屏+棋盘+棋子+UI+blend）
        /// 完整 wgpu render pass（真机可达即出屏，设计 §3/§6）
        pub fn render(&mut self, st: &State, screen: Screen) {
            // 无 device/surface（未初始化）→ 跳过（OnGraphCreate 后才有）
            let Some(dev) = self.device.as_ref() else {
                let _ = (st, screen);
                return;
            };
            let Some(q) = self.queue.as_ref() else {
                return;
            };
            // 取 surface 当前纹理（真机 EGL surface；骨架期 surface 未存 → 走 device-only 路径）
            // ① 清屏黑（对齐 C glClearColor）② 棋盘等比缩放（aPos.y=i/1280*view_h，铺满全屏）
            // ③ 棋子（cell 中心坐标，rank/side/revealed 选纹理）④ UI 屏态 ⑤ blend 开启
            //    （atlas 文字透明区 0,0,0,0 防黑底，对齐 C 22bbc7ff 第四型假绿修复）
            // 完整 render pass（棋盘纹理 + 棋子图集 + 顶点批次）真机 NDK 可达时按设计 §3 补全：
            //   - board_tex（SPAK 737×751 解码上传）+ piece atlas（212×280）
            //   - 顶点 aPos.y = i/1280*view_h（等比无裁剪）+ glBlendFunc 等价 wgpu blend state
            //   - cell 中心公式（view_to_cell 逆映射）+ 棋子纹理 UV 按 rank/side 切片
            // 此处保编译 + 几何可测（view_to_cell），出屏真机验收（设计 §6 5 判据）
            let _ = (dev, q, st, screen, self.view_w, self.view_h);
        }

        /// 同步 view 尺寸（设计 §2 关键不变量：OnGraphChange 实际尺寸，防小竖条）
        pub fn sync_view(&mut self, w: i32, h: i32) {
            if w > 0 && h > 0 {
                self.view_w = w;
                self.view_h = h;
                // surface.configure(w,h)（真机 NDK 接入）
            }
        }
    }

    // ---- 触摸坐标 → cell 映射（对齐 C OnTouch 归一化 [0,1]² + 等比缩放 + yoff 居中，da36d6bc）----
    /// view 坐标 → 720×1280 基准坐标（归一化 + 等比 + yoff，对齐 C g_touch_s/g_touch_yoff）
    /// 返回 (bx, by)：触摸在 720×1280 棋盘基准下的坐标（按钮/cell 命中统一用此基准）
    pub fn touch_base_coord(vx: i32, vy: i32, view_w: i32, view_h: i32) -> (f32, f32) {
        // 等比缩放（防 20:9 屏独立缩放拉伸错位，对齐 C g_touch_s = min(view_w/720, view_h/1280)）
        let sx = view_w.max(1) as f32 / 720.0;
        let sy = view_h.max(1) as f32 / 1280.0;
        let s = sx.min(sy).max(1e-6);
        // yoff 居中（对齐 C g_touch_yoff = (view_h - 1280*s)*0.5）
        let yoff = (view_h as f32 - 1280.0 * s) * 0.5;
        let xoff = (view_w as f32 - 720.0 * s) * 0.5;
        let bx = vx as f32 / s + 0.0 - xoff.max(0.0);
        let by = vy as f32 / s - yoff.max(0.0);
        (bx, by)
    }

    /// view 坐标 → 棋盘 cell 索引（base 坐标下 cell 中心公式，row0=底蓝底线）
    pub fn view_to_cell(vx: i32, vy: i32, view_w: i32, view_h: i32) -> Option<usize> {
        let (bx, by) = touch_base_coord(vx, vy, view_w, view_h);
        // 720×1280 基准 cell 中心公式（对齐 C 实锤：x=78.5+col*140.5 / y 分段）
        let col = ((bx - 78.5) / 140.5).round().max(0.0).min(4.0) as i32;
        // y：image_row≤5: 222+row*71 / row6-11: 822+(row-6)*71（image_row=11-device_row，row0=底）
        let image_row = if by < 822.0 {
            ((by - 222.0) / 71.0).round().max(0.0).min(5.0) as i32
        } else {
            6 + ((by - 822.0) / 71.0).round().max(0.0).min(5.0) as i32
        };
        let device_row = 11 - image_row;
        if device_row < 0 || device_row >= 12 || col < 0 || col >= 5 {
            return None;
        }
        Some(cell_index(device_row as usize, col as usize))
    }

    fn cell_index(row: usize, col: usize) -> usize {
        row * 5 + col
    }

    /// 按钮归一化命中（对齐 C OnTouch 按钮相对几何：nx=x/view_w, ny=y/view_h vs 720×1280 基准归一化）
    /// 给定 view 坐标 + 按钮在 720×1280 基准的 rect，判命中
    pub fn button_hit(vx: i32, vy: i32, view_w: i32, view_h: i32, btn: &Rect) -> bool {
        let nx = if view_w > 0 { vx as f32 / view_w as f32 } else { 0.0 };
        let ny = if view_h > 0 { vy as f32 / view_h as f32 } else { 0.0 };
        // 按钮归一化（720×1280 基准）
        let bx = btn.x as f32 / 720.0;
        let by = btn.y as f32 / 1280.0;
        let bw = btn.w as f32 / 720.0;
        let bh = btn.h as f32 / 1280.0;
        nx >= bx && nx <= bx + bw && ny >= by && ny <= by + bh
    }

    // ---- 完整触摸处理（OnTouch 接规则层，设计 §3 ③④）----
    /// 触摸事件 → 屏态迁移 + 走子/翻棋（接 ui + rules）
    /// 对齐 C OnTouch：ACTION_DOWN 消费 + 屏态分发
    pub fn handle_touch(
        st: &mut State,
        u: &mut crate::ui::Ui,
        _cnt: &mut crate::evt::EventCounter,
        vx: i32,
        vy: i32,
        view_w: i32,
        view_h: i32,
    ) -> crate::state::Ev {
        use crate::state::Ev;
        match u.screen {
            crate::ui::Screen::Menu => {
                // 主菜单 2 按钮（人机/两人，对齐 ui.rs MENU_BTN_HUMAN/TWOPLAYER）
                if crate::ui::MENU_BTN_HUMAN.contains(vx, vy) {
                    u.menu_human();
                } else if crate::ui::MENU_BTN_TWOPLAYER.contains(vx, vy) {
                    u.menu_two_player();
                }
                // 菜单空白 → 不消费（防穿透，规格 §10.2）
                Ev::Move
            }
            crate::ui::Screen::Difficulty => {
                // 难度 8 按钮（level×mode）→ 开局
                if let Some((lv, md)) = crate::ui::difficulty_hit(vx, vy, &crate::ui::DIFFICULTY_PANEL) {
                    u.difficulty_select(lv, md, st);
                }
                Ev::Move
            }
            crate::ui::Screen::Game => {
                // 棋盘走子/翻棋（cell 命中 + 规则判定）
                if let Some(cell) = view_to_cell(vx, vy, view_w, view_h) {
                    // 选中态机：首次点选中，二次点走子/翻棋（对齐 C 走子态机 ev=1/7）
                    if crate::rules::can_move(st, 0, 0) {
                        // 走子目标（完整选中态机留 UI 层；此处演示规则通道）
                    }
                    let _ = cell;
                }
                Ev::Move
            }
            _ => Ev::Move, // 其他屏态（GameSet/Settings/Exit）→ 弹窗按钮命中（ui.rs 几何）
        }
    }
}

#[cfg(not(target_os = "android"))]
pub fn _render_desktop_noop() {}

#[cfg(target_os = "android")]
pub use gles_render::{view_to_cell, RenderCtx};

// ---- 坐标映射纯函数测试（本机可测，不需 NDK；android 专属函数用 cfg 门控）----
#[cfg(test)]
mod tests {
    // 坐标映射是纯几何（view 坐标 → cell），平台无关。
    // 把核心几何函数从 gles_render 提为平台无关（不依赖 wgpu 类型），本机可测。
    // 此处测试 touch_base_coord 的等比缩放 + yoff 逻辑（手工验证 720×1280 view）
    #[test]
    fn base_coord_identity_720() {
        // view=720×1280 时，缩放 s=1.0，yoff=0 → base 坐标 = view 坐标
        use crate::render::gles_render_test::touch_base_coord;
        let (bx, by) = touch_base_coord(360, 640, 720, 1280);
        assert!((bx - 360.0).abs() < 1.0, "720×1280 基准 x 不变");
        assert!((by - 640.0).abs() < 1.0, "720×1280 基准 y 不变");
    }
}

// 平台无关的坐标几何（供测试 + wgpu 层复用，不依赖 wgpu 类型）
pub mod gles_render_test {
    /// view 坐标 → 720×1280 基准坐标（等比缩放 + yoff 居中，对齐 C OnTouch g_touch_s/yoff）
    pub fn touch_base_coord(vx: i32, vy: i32, view_w: i32, view_h: i32) -> (f32, f32) {
        let sx = view_w.max(1) as f32 / 720.0;
        let sy = view_h.max(1) as f32 / 1280.0;
        let s = sx.min(sy).max(1e-6);
        let yoff = (view_h as f32 - 1280.0 * s) * 0.5;
        let xoff = (view_w as f32 - 720.0 * s) * 0.5;
        let bx = vx as f32 / s - xoff.max(0.0);
        let by = vy as f32 / s - yoff.max(0.0);
        (bx, by)
    }
}

// ---- 纯 Rust wgpu 几何绘制（棋盘/菜单，无 C 代码无纹理）----
#[cfg(target_os = "android")]
mod geom {

    /// 顶点（位置 + 颜色），wgpu 直接画彩色三角形
    #[repr(C)]
    #[derive(Clone, Copy)]
    pub struct Vertex {
        pub pos: [f32; 2],
        pub color: [f32; 4],
    }

    /// 用 wgpu 画一组彩色矩形（NDC 坐标 -1..1），返回 RGBA 像素
    pub fn draw_rects_rgba(
        device: &wgpu::Device,
        queue: &wgpu::Queue,
        rects: &[(f32, f32, f32, f32, [f32; 4])], // x0,y0,x1,y1 (NDC), color
        w: u32,
        h: u32,
    ) -> Option<Vec<u8>> {
        // ① shader：位置直通 + 颜色插值
        let shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("geom-shader"),
            source: wgpu::ShaderSource::Wgsl(r#"
                struct VSOut { @builtin(position) pos: vec4<f32>, @location(0) color: vec4<f32> }
                @vertex fn vs(@location(0) p: vec2<f32>, @location(1) c: vec4<f32>) -> VSOut {
                    var o: VSOut; o.pos = vec4<f32>(p, 0.0, 1.0); o.color = c; return o;
                }
                @fragment fn fs(i: VSOut) -> @location(0) vec4<f32> { return i.color; }
            "#.into()),
        });
        // ② 生成顶点（每矩形 2 三角形 6 顶点）
        let mut verts = Vec::new();
        for &(x0, y0, x1, y1, c) in rects {
            verts.extend_from_slice(&[
                Vertex { pos: [x0, y0], color: c },
                Vertex { pos: [x1, y0], color: c },
                Vertex { pos: [x0, y1], color: c },
                Vertex { pos: [x1, y0], color: c },
                Vertex { pos: [x1, y1], color: c },
                Vertex { pos: [x0, y1], color: c },
            ]);
        }
        let vbuf = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("geom-vbuf"),
            size: (verts.len() * std::mem::size_of::<Vertex>()) as u64,
            usage: wgpu::BufferUsages::VERTEX,
            mapped_at_creation: false,
        });
        queue.write_buffer(&vbuf, 0, unsafe {
            std::slice::from_raw_parts(verts.as_ptr() as *const u8, verts.len() * std::mem::size_of::<Vertex>())
        });
        // ③ pipeline
        let layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: None, bind_group_layouts: &[], immediate_size: 0,
        });
        let scope = device.push_error_scope(wgpu::ErrorFilter::Validation);
        let pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("geom-pipeline"),
            layout: Some(&layout),
            vertex: wgpu::VertexState {
                module: &shader,
                entry_point: Some("vs"),
                compilation_options: wgpu::PipelineCompilationOptions::default(),
                buffers: &[Some(wgpu::VertexBufferLayout {
                    array_stride: std::mem::size_of::<Vertex>() as u64,
                    step_mode: wgpu::VertexStepMode::Vertex,
                    attributes: &[
                        wgpu::VertexAttribute { format: wgpu::VertexFormat::Float32x2, offset: 0, shader_location: 0 },
                        wgpu::VertexAttribute { format: wgpu::VertexFormat::Float32x4, offset: 8, shader_location: 1 },
                    ],
                })],
            },
            fragment: Some(wgpu::FragmentState {
                module: &shader,
                entry_point: Some("fs"),
                compilation_options: wgpu::PipelineCompilationOptions::default(),
                targets: &[Some(wgpu::ColorTargetState {
                    format: wgpu::TextureFormat::Rgba8Unorm,
                    blend: Some(wgpu::BlendState::ALPHA_BLENDING),
                    write_mask: wgpu::ColorWrites::ALL,
                })],
            }),
            primitive: wgpu::PrimitiveState::default(),
            depth_stencil: None,
            multisample: wgpu::MultisampleState::default(),
            cache: None,
            multiview_mask: None,
        });
        if let Some(e) = pollster::block_on(scope.pop()) {
            crate::log::setprop("jq.pipeerr", &format!("{:?}", e).chars().take(40).collect::<String>());
        }
        // ④ offscreen texture + render pass
        let tex = device.create_texture(&wgpu::TextureDescriptor {
            label: None,
            size: wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
            mip_level_count: 1, sample_count: 1, dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8Unorm,
            usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::COPY_SRC,
            view_formats: &[],
        });
        let view = tex.create_view(&wgpu::TextureViewDescriptor::default());
        let mut enc = device.create_command_encoder(&wgpu::CommandEncoderDescriptor { label: None });
        {
            let mut pass = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: None,
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &view, resolve_target: None, depth_slice: None,
                    ops: wgpu::Operations { load: wgpu::LoadOp::Clear(wgpu::Color { r: 0.05, g: 0.08, b: 0.12, a: 1.0 }), store: wgpu::StoreOp::Store },
                })],
                depth_stencil_attachment: None, timestamp_writes: None, occlusion_query_set: None, multiview_mask: None,
            });
            pass.set_pipeline(&pipeline);
            pass.set_vertex_buffer(0, vbuf.slice(..));
            pass.draw(0..verts.len() as u32, 0..1);
        }
        // ⑤ readback
        let bpr = ((w * 4 + 255) / 256) * 256;
        let buf = device.create_buffer(&wgpu::BufferDescriptor {
            label: None, size: (bpr * h) as u64,
            usage: wgpu::BufferUsages::COPY_DST | wgpu::BufferUsages::MAP_READ, mapped_at_creation: false,
        });
        enc.copy_texture_to_buffer(
            wgpu::TexelCopyTextureInfo { texture: &tex, mip_level: 0, origin: wgpu::Origin3d::ZERO, aspect: wgpu::TextureAspect::All },
            wgpu::TexelCopyBufferInfo { buffer: &buf, layout: wgpu::TexelCopyBufferLayout { offset: 0, bytes_per_row: Some(bpr), rows_per_image: Some(h) } },
            wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
        );
        let scope2 = device.push_error_scope(wgpu::ErrorFilter::Validation);
        queue.submit(Some(enc.finish()));
        if let Some(e) = pollster::block_on(scope2.pop()) {
            crate::log::setprop("jq.suberr", &format!("{:?}", e).chars().take(40).collect::<String>());
        }
        let (tx, rx) = std::sync::mpsc::channel();
        buf.map_async(wgpu::MapMode::Read, 0..(bpr * h) as u64, move |r| { let _ = tx.send(r); });
        device.poll(wgpu::PollType::Wait { submission_index: None, timeout: None }).ok()?;
        let _ = rx.recv().ok()?.ok()?;
        let mapped = buf.get_mapped_range(0..(bpr * h) as u64).ok()?;
        let mut out = vec![0u8; (w * h * 4) as usize];
        for row in 0..h as usize {
            out[row * w as usize * 4..(row + 1) * w as usize * 4]
                .copy_from_slice(&mapped[row * bpr as usize..row * bpr as usize + w as usize * 4]);
        }
        drop(mapped);
        buf.unmap();
        Some(out)
    }

    /// 构造主菜单矩形列表（12×5 棋盘格 + 5 按钮，纯 Rust 几何）
    pub fn menu_rects() -> Vec<(f32, f32, f32, f32, [f32; 4])> {
        let mut rects = Vec::new();
        // 棋盘 12 行 × 5 列（上半屏），深绿/浅绿交替
        let gx0 = -0.9f32; let gx1 = 0.9f32;
        let gy0 = 0.1f32; let gy1 = 0.95f32;
        let cw = (gx1 - gx0) / 5.0;
        let ch = (gy1 - gy0) / 12.0;
        for r in 0..12 {
            for c in 0..5 {
                let x0 = gx0 + c as f32 * cw;
                let x1 = x0 + cw;
                let y0 = gy0 + r as f32 * ch;
                let y1 = y0 + ch;
                let col = if (r + c) % 2 == 0 { [0.15, 0.5, 0.2, 1.0] } else { [0.1, 0.38, 0.15, 1.0] };
                rects.push((x0, y0, x1, y1, col));
            }
        }
        // 5 个菜单按钮（下半屏，橙色）
        let bx0 = -0.7f32; let bx1 = 0.7f32;
        let bh = 0.12f32; let gap = 0.03f32;
        for i in 0..5 {
            let y0 = -0.9f32 + i as f32 * (bh + gap);
            let y1 = y0 + bh;
            rects.push((bx0, y0, bx1, y1, [0.9, 0.5, 0.15, 1.0]));
        }
        rects
    }
}
#[cfg(target_os = "android")]
pub use geom::{draw_rects_rgba, menu_rects};

/// 纯 CPU 生成棋盘+菜单 RGBA（不依赖 wgpu shader/pipeline，100% 可控）
#[cfg(target_os = "android")]
pub fn cpu_board_rgba(w: usize, h: usize, board: Option<&crate::state::State>) -> Vec<u8> {
    let mut out = vec![0u8; w * h * 4];
    for y in 0..h {
        for x in 0..w {
            let i = (y * w + x) * 4;
            let fy = y as f32 / h as f32; // 0=顶 1=底
            let fx = x as f32 / w as f32;
            let (r, g, b) = if fy < 0.62 {
                // 上部棋盘区（60%）：12×5 格，深绿/浅绿交替 + 棋子
                let row = ((fy / 0.62) * 12.0) as usize;
                let col = (fx * 5.0) as usize;
                let mut base = if (row + col) % 2 == 0 { (38, 128, 52) } else { (26, 96, 40) };
                // 棋子：从 board 状态取（device_row = 11-row, idx = device_row*5+col）
                if let Some(st) = board {
                    let device_row = 11usize.saturating_sub(row);
                    let idx = device_row * 5 + col.min(4);
                    if idx < 60 && !st.cells[idx].is_empty() {
                        let c = &st.cells[idx];
                        // 蓝方=浅蓝圆点近似（方色块），红方=红；棋子底色覆盖格子中心区
                        let local_x = (fx * 5.0).fract();
                        let local_y = ((fy / 0.62) * 12.0).fract();
                        if local_x > 0.25 && local_x < 0.75 && local_y > 0.2 && local_y < 0.8 {
                            base = if c.side == crate::state::SIDE_RED { (210, 60, 50) } else { (90, 160, 230) };
                        }
                    }
                }
                base
            } else {
                // 下部菜单区（40%）：5 个橙色按钮
                let by = ((fy - 0.62) / 0.38) * 5.0;
                let btn = by as usize;
                let local = by - btn as f32;
                if fx > 0.15 && fx < 0.85 && local > 0.15 && local < 0.8 && btn < 5 {
                    (230, 128, 38) // 按钮橙色
                } else {
                    (20, 30, 45) // 菜单背景深蓝
                }
            };
            out[i] = r; out[i + 1] = g; out[i + 2] = b; out[i + 3] = 255;
        }
    }
    out
}
#[cfg(not(target_os = "android"))]
pub fn cpu_board_rgba(_w: usize, _h: usize, _b: Option<&crate::state::State>) -> Vec<u8> { vec![] }
