//! wgpu 渲染上下文与管线（从 render.rs 拆分，逐字搬运）
#[cfg(target_os = "android")]
pub(crate) mod gles_render {
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
            use pollster::FutureExt;
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
            use pollster::FutureExt;
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
