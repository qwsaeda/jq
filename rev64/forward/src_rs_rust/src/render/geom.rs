//! 顶点/矩形几何 + RGBA 合成（从 render.rs 拆分，逐字搬运）
#[cfg(target_os = "android")]
pub(crate) mod geom {

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
