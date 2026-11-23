//! gpu — 真正的 wgpu GPU 渲染管线（shader + pipeline + 纹理 + vertex batch + blend + offscreen readback）
//! 设计依据：WGPU_RENDER_DESIGN.md §1-§3（wgpu 30 / gles 后端 / blend 必须真生效）。
//! ★ 与旧实现的区别：旧代码 offscreen 只 clear 纯色，CPU 合成画面再由 Canvas 显示（GPU 不参与场景生成）；
//!   本模块让 GPU 直接按 asset::build_scene 的 quad 列表绘制棋盘/棋子/UI，readback 后交 Canvas 显示。
//! ★ houdini 下 wgpu EGL window surface 创建失败（JS 文档 §4 已实锤），因此采用 offscreen texture 出屏：
//!   这是 GPU 渲染管线（不是 CPU 合成 + GPU blit），场景几何与纹理采样全部在 shader 中完成。
//! ★ 假绿防线（CODE_STYLE §9/§10）：每阶段写 jq.gpu_state 属性 + Validation ErrorScope 捕获到 jq.gpu_err；
//!   失败必须显式降级到 CPU 合成并写 jq.gpu_state=cpu_fallback，禁止"静默不画还报成功"。
#![cfg(target_os = "android")]
use std::collections::HashMap;

use super::quad::{Quad, scene_quads};
use crate::asset::Tex;
use crate::log::setprop;

/// 顶点：NDC pos(2) + UV(2) + tint(4) = 8 f32 = 32B
#[repr(C)]
#[derive(Clone, Copy)]
struct Vertex {
    pos: [f32; 2],
    uv: [f32; 2],
    tint: [f32; 4],
}

const WGSL: &str = r#"
struct VSOut {
    @builtin(position) pos: vec4<f32>,
    @location(0) uv: vec2<f32>,
    @location(1) tint: vec4<f32>,
};
@vertex fn vs(
    @location(0) p: vec2<f32>,
    @location(1) uv: vec2<f32>,
    @location(2) tint: vec4<f32>,
) -> VSOut {
    var o: VSOut;
    o.pos = vec4<f32>(p, 0.0, 1.0);
    o.uv = uv;
    o.tint = tint;
    return o;
}
@group(0) @binding(0) var samp: sampler;
@group(0) @binding(1) var tex: texture_2d<f32>;
@fragment fn fs(i: VSOut) -> @location(0) vec4<f32> {
    let t = textureSample(tex, samp, i.uv);
    // 直 alpha 混合（BlendState::ALPHA_BLENDING = src.rgb*src.a + dst.rgb*(1-src.a)）
    return vec4<f32>(t.rgb * i.tint.rgb, t.a * i.tint.a);
}
"#;

pub(crate) struct GpuRenderer {
    device: wgpu::Device,
    queue: wgpu::Queue,
    pipeline: wgpu::RenderPipeline,
    sampler: wgpu::Sampler,
    bgl: wgpu::BindGroupLayout,
    bind_groups: HashMap<Tex, wgpu::BindGroup>,
    tex_sizes: HashMap<Tex, (u32, u32)>,
    vbuf: wgpu::Buffer,
    vbuf_bytes: u64,
    offscreen: Option<(wgpu::Texture, u32, u32)>,
    readback: Option<wgpu::Buffer>,
    readback_bytes: u64,
}

impl GpuRenderer {
    /// 创建 device + pipeline + 纹理（app 生命周期内一次），失败返回 None（调用方降级 CPU）
    pub(crate) fn init() -> Option<Self> {
        let (device, queue) = match crate::render::RenderCtx::create_wgpu_device() {
            Some(v) => v,
            None => { setprop("jq.gpu_state", "device_fail"); return None; }
        };
        setprop("jq.gpu_state", "device_ok");

        let shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("junqi-scene-shader"),
            source: wgpu::ShaderSource::Wgsl(WGSL.into()),
        });
        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            label: Some("junqi-sampler"),
            address_mode_u: wgpu::AddressMode::ClampToEdge,
            address_mode_v: wgpu::AddressMode::ClampToEdge,
            address_mode_w: wgpu::AddressMode::ClampToEdge,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            mipmap_filter: wgpu::MipmapFilterMode::Linear,
            ..Default::default()
        });
        let bgl = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("junqi-bgl"),
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 1,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        sample_type: wgpu::TextureSampleType::Float { filterable: true },
                        view_dimension: wgpu::TextureViewDimension::D2,
                        multisampled: false,
                    },
                    count: None,
                },
            ],
        });
        let layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("junqi-pipeline-layout"),
            bind_group_layouts: &[Some(&bgl)],
            immediate_size: 0,
        });
        // ★ 假绿防线：pipeline 创建失败必须被 ErrorScope 抓到并写 jq.gpu_err
        let scope = device.push_error_scope(wgpu::ErrorFilter::Validation);
        let pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("junqi-scene-pipeline"),
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
                        wgpu::VertexAttribute { format: wgpu::VertexFormat::Float32x2, offset: 8, shader_location: 1 },
                        wgpu::VertexAttribute { format: wgpu::VertexFormat::Float32x4, offset: 16, shader_location: 2 },
                    ],
                })],
            },
            fragment: Some(wgpu::FragmentState {
                module: &shader,
                entry_point: Some("fs"),
                compilation_options: wgpu::PipelineCompilationOptions::default(),
                targets: &[Some(wgpu::ColorTargetState {
                    format: wgpu::TextureFormat::Rgba8Unorm,
                    // ★ WGPU_RENDER_DESIGN §3 ⑤ blend 必须真生效（C 22bbc7ff 第四型假绿：透明区画成黑底）
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
            setprop("jq.gpu_err", &format!("pipeline:{:?}", e).chars().take(60).collect::<String>());
            setprop("jq.gpu_state", "pipeline_err");
            return None;
        }
        setprop("jq.gpu_state", "pipeline_ok");

        let vbuf = device.create_buffer(&wgpu::BufferDescriptor {
            label: Some("junqi-vbuf-init"), size: 4096,
            usage: wgpu::BufferUsages::VERTEX | wgpu::BufferUsages::COPY_DST,
            mapped_at_creation: false,
        });
        let mut r = Self {
            device, queue, pipeline, sampler, bgl,
            bind_groups: HashMap::new(), tex_sizes: HashMap::new(),
            vbuf, vbuf_bytes: 4096,
            offscreen: None, readback: None, readback_bytes: 0,
        };
        // 预热全部纹理（解码一次 + 上传一次，跨帧复用）
        for &tex in ALL_TEX {
            if let Some((rgba, w, h)) = Self::tex_bytes(tex) {
                let rgba = Self::preprocess(tex, rgba, w, h);
                let view = Self::upload_texture(&r.device, &r.queue, &rgba, w, h, tex);
                let bg = r.device.create_bind_group(&wgpu::BindGroupDescriptor {
                    label: Some("junqi-bindgroup"),
                    layout: &r.bgl,
                    entries: &[
                        wgpu::BindGroupEntry { binding: 0, resource: wgpu::BindingResource::Sampler(&r.sampler) },
                        wgpu::BindGroupEntry { binding: 1, resource: wgpu::BindingResource::TextureView(&view) },
                    ],
                });
                r.bind_groups.insert(tex, bg);
                r.tex_sizes.insert(tex, (w, h));
            }
        }
        if r.bind_groups.is_empty() {
            setprop("jq.gpu_state", "texture_fail");
            return None;
        }
        setprop("jq.gpu_state", "textures_ok");
        Some(r)
    }

    fn tex_bytes(tex: Tex) -> Option<(Vec<u8>, u32, u32)> {
        if tex == Tex::White {
            return Some((vec![255, 255, 255, 255], 1, 1));
        }
        crate::asset::texture_rgba(tex).map(|(v, w, h)| (v.clone(), *w, *h))
    }

    /// 棋子纹理预乘 alpha 遮罩：正面椭圆 + 去分隔线；背面方块 + 去底部 3px（对齐 CPU piece 语义）
    fn preprocess(tex: Tex, mut rgba: Vec<u8>, w: u32, h: u32) -> Vec<u8> {
        let (front, back) = match tex {
            Tex::PieceFront => (true, false),
            Tex::PieceBack => (false, true),
            _ => return rgba,
        };
        let cw = (w / 4) as usize;
        let ch = (h / 7) as usize;
        for cell in 0..28usize {
            let cx0 = (cell % 4) * cw;
            let cy0 = (cell / 4) * ch;
            for py in 0..ch {
                for px in 0..cw {
                    let i = ((cy0 + py) * w as usize + cx0 + px) * 4 + 3;
                    if i >= rgba.len() { continue; }
                    let nx = px as f32 / cw as f32;
                    let ny = py as f32 / ch as f32;
                    if front {
                        let inside = ((nx - 0.5).powi(2) / 0.22 + (ny - 0.5).powi(2) / 0.24) < 1.0;
                        let bright = rgba[i-3] as u32 + rgba[i-2] as u32 + rgba[i-1] as u32 > 120;
                        if !inside || !bright { rgba[i] = 0; }
                    } else if back && py >= ch.saturating_sub(3) {
                        rgba[i] = 0;
                    }
                }
            }
        }
        rgba
    }

    fn upload_texture(device: &wgpu::Device, queue: &wgpu::Queue,
                      rgba: &[u8], w: u32, h: u32, tex: Tex) -> wgpu::TextureView {
        let bpr = ((w * 4 + 255) / 256) * 256;
        let mut padded = vec![0u8; (bpr * h) as usize];
        for y in 0..h as usize {
            let src = &rgba[y * w as usize * 4..(y + 1) * w as usize * 4];
            padded[y * bpr as usize..y * bpr as usize + w as usize * 4].copy_from_slice(src);
        }
        let texture = device.create_texture(&wgpu::TextureDescriptor {
            label: Some(match tex { Tex::BoardBg => "junqi-board-bg", _ => "junqi-asset" }),
            size: wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
            mip_level_count: 1, sample_count: 1, dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::Rgba8Unorm,
            usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
            view_formats: &[],
        });
        queue.write_texture(
            wgpu::TexelCopyTextureInfo {
                texture: &texture, mip_level: 0, origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            &padded,
            wgpu::TexelCopyBufferLayout { offset: 0, bytes_per_row: Some(bpr), rows_per_image: Some(h) },
            wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
        );
        texture.create_view(&wgpu::TextureViewDescriptor::default())
    }

    /// 按 quad 列表渲染一帧 offscreen RGBA（w×h）
    pub(crate) fn render_frame(&mut self, quads: &[Quad], w: u32, h: u32) -> Option<Vec<u8>> {
        if quads.is_empty() { return None; }
        let verts = self.build_vertices(quads, w, h)?;
        let need = (verts.len() * std::mem::size_of::<Vertex>()) as u64;
        if need > self.vbuf_bytes {
            self.vbuf = self.device.create_buffer(&wgpu::BufferDescriptor {
                label: Some("junqi-vbuf"),
                size: need.max(4096),
                usage: wgpu::BufferUsages::VERTEX | wgpu::BufferUsages::COPY_DST,
                mapped_at_creation: false,
            });
            self.vbuf_bytes = need.max(4096);
        }
        self.queue.write_buffer(&self.vbuf, 0, unsafe {
            std::slice::from_raw_parts(verts.as_ptr() as *const u8, verts.len() * std::mem::size_of::<Vertex>())
        });

        // offscreen texture 按需重建（OnGraphChange 尺寸同步）
        let size_changed = self.offscreen.as_ref().map(|(_, ow, oh)| *ow != w || *oh != h).unwrap_or(true);
        if size_changed {
            let tex = self.device.create_texture(&wgpu::TextureDescriptor {
                label: Some("junqi-offscreen"),
                size: wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
                mip_level_count: 1, sample_count: 1, dimension: wgpu::TextureDimension::D2,
                format: wgpu::TextureFormat::Rgba8Unorm,
                usage: wgpu::TextureUsages::RENDER_ATTACHMENT | wgpu::TextureUsages::COPY_SRC,
                view_formats: &[],
            });
            self.offscreen = Some((tex, w, h));
        }
        let (offscreen, _, _) = self.offscreen.as_ref().unwrap();
        let view = offscreen.create_view(&wgpu::TextureViewDescriptor::default());

        let scope = self.device.push_error_scope(wgpu::ErrorFilter::Validation);
        let mut enc = self.device.create_command_encoder(&wgpu::CommandEncoderDescriptor { label: None });
        {
            let mut pass = enc.begin_render_pass(&wgpu::RenderPassDescriptor {
                label: Some("junqi-scene-pass"),
                color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &view, resolve_target: None, depth_slice: None,
                    ops: wgpu::Operations {
                        // C glClearColor 对齐：清黑，背景纹理随后铺满
                        load: wgpu::LoadOp::Clear(wgpu::Color { r: 0.0, g: 0.0, b: 0.0, a: 1.0 }),
                        store: wgpu::StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None, timestamp_writes: None,
                occlusion_query_set: None, multiview_mask: None,
            });
            pass.set_pipeline(&self.pipeline);
            // 按纹理连续段分组提交（保持图层顺序 + 减少 bind/draw 次数）
            for (tex, start, count) in super::quad::quad_batches(quads) {
                let Some(bg) = self.bind_groups.get(&tex) else {
                    setprop("jq.gpu_err", "missing_bindgroup");
                    return None;
                };
                pass.set_bind_group(0, bg, &[]);
                let vs = start * 6;
                let vc = count * 6;
                let off = (vs * std::mem::size_of::<Vertex>()) as u64;
                let len = (vc * std::mem::size_of::<Vertex>()) as u64;
                pass.set_vertex_buffer(0, self.vbuf.slice(off..off + len));
                pass.draw(0..vc as u32, 0..1);
            }
        }
        let bpr = ((w * 4 + 255) / 256) * 256;
        let rb_bytes = (bpr * h) as u64;
        if self.readback.as_ref().map(|b| b.size() != rb_bytes).unwrap_or(true) {
            self.readback = Some(self.device.create_buffer(&wgpu::BufferDescriptor {
                label: Some("junqi-readback"), size: rb_bytes,
                usage: wgpu::BufferUsages::COPY_DST | wgpu::BufferUsages::MAP_READ,
                mapped_at_creation: false,
            }));
            self.readback_bytes = rb_bytes;
        }
        let rb = self.readback.as_ref().unwrap();
        enc.copy_texture_to_buffer(
            wgpu::TexelCopyTextureInfo {
                texture: offscreen, mip_level: 0, origin: wgpu::Origin3d::ZERO,
                aspect: wgpu::TextureAspect::All,
            },
            wgpu::TexelCopyBufferInfo {
                buffer: rb,
                layout: wgpu::TexelCopyBufferLayout {
                    offset: 0, bytes_per_row: Some(bpr), rows_per_image: Some(h),
                },
            },
            wgpu::Extent3d { width: w, height: h, depth_or_array_layers: 1 },
        );
        self.queue.submit(Some(enc.finish()));
        if let Some(e) = pollster::block_on(scope.pop()) {
            setprop("jq.gpu_err", &format!("render:{:?}", e).chars().take(60).collect::<String>());
            setprop("jq.gpu_state", "render_err");
            return None;
        }

        let (tx, rx) = std::sync::mpsc::channel();
        rb.map_async(wgpu::MapMode::Read, 0..rb_bytes, move |r| { let _ = tx.send(r); });
        if self.device.poll(wgpu::PollType::Wait { submission_index: None, timeout: None }).is_err() {
            setprop("jq.gpu_state", "poll_err");
            return None;
        }
        let Ok(Ok(())) = rx.recv() else {
            setprop("jq.gpu_state", "map_err");
            return None;
        };
        let mapped = rb.get_mapped_range(0..rb_bytes).ok()?;
        let mut out = vec![0u8; (w * h * 4) as usize];
        for y in 0..h as usize {
            let src = &mapped[y * bpr as usize..y * bpr as usize + w as usize * 4];
            out[y * w as usize * 4..(y + 1) * w as usize * 4].copy_from_slice(src);
        }
        drop(mapped);
        rb.unmap();
        let _ = self.readback_bytes;
        setprop("jq.gpu_state", "render_ok");
        Some(out)
    }

    fn build_vertices(&self, quads: &[Quad], w: u32, h: u32) -> Option<Vec<Vertex>> {
        let mut out = Vec::with_capacity(quads.len() * 6);
        for q in quads {
            let (tw, th) = *self.tex_sizes.get(&q.tex)?;
            let u0 = (q.sx0 as f32 + 0.5) / tw as f32;
            let u1 = (q.sx0 as f32 + q.sw as f32 - 0.5) / tw as f32;
            let v0 = (q.sy0 as f32 + 0.5) / th as f32;
            let v1 = (q.sy0 as f32 + q.sh as f32 - 0.5) / th as f32;
            let ndc = |x: f32, y: f32| [x / w as f32 * 2.0 - 1.0, 1.0 - y / h as f32 * 2.0];
            let p00 = ndc(q.dx0, q.dy0);
            let p10 = ndc(q.dx0 + q.dw, q.dy0);
            let p01 = ndc(q.dx0, q.dy0 + q.dh);
            let p11 = ndc(q.dx0 + q.dw, q.dy0 + q.dh);
            let t = q.tint;
            out.extend_from_slice(&[
                Vertex { pos: p00, uv: [u0, v0], tint: t },
                Vertex { pos: p10, uv: [u1, v0], tint: t },
                Vertex { pos: p01, uv: [u0, v1], tint: t },
                Vertex { pos: p10, uv: [u1, v0], tint: t },
                Vertex { pos: p11, uv: [u1, v1], tint: t },
                Vertex { pos: p01, uv: [u0, v1], tint: t },
            ]);
        }
        Some(out)
    }
}

/// ★ 必须与 asset::Tex 全量同步：新增纹理未预热 → build_vertices 取不到尺寸 →
///   该帧 GPU 渲染失败并静默降级 CPU（CODE_STYLE §9 静默降级假绿）。
///   asset.rs 增加 Tex 变体时，此处必须同补（编译期靠 exhaustiveness 无法保证，靠注释纪律 + 测试）。
const ALL_TEX: &[Tex] = &[
    Tex::White, Tex::BoardBg, Tex::MenuBg, Tex::MenuBig, Tex::MenuSmall, Tex::House,
    Tex::PieceFront, Tex::PieceBack, Tex::ImenIcons, Tex::ImenText, Tex::ImenHamburger,
    Tex::DiffText, Tex::GameSetText, Tex::NewGameText, Tex::ExitText, Tex::SoundText,
    Tex::GameInfoText,
];



/// 渲染一帧的便捷入口：GPU 失败时返回 None（调用方按 jq.gpu_state 决定是否降级）
pub(crate) fn render_scene(r: &mut GpuRenderer, w: u32, h: u32,
                    board: Option<&crate::state::State>,
                    screen: crate::ui::Screen,
                    stats: Option<&crate::evt::EventCounter>) -> Option<Vec<u8>> {
    let quads = scene_quads(w as usize, h as usize, board, screen, stats);
    r.render_frame(&quads, w, h)
}
