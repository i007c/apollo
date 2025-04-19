use crate::model::{INDICES, NORMALS, Normal, POSITIONS, Position};
use cgmath::{Matrix3, Matrix4, Point3, Rad, Vector3};
use egui_winit_vulkano::egui;
use std::{process::Command, sync::Arc, time::Instant};
use vulkano::{
    Validated, VulkanError,
    buffer::{
        Buffer, BufferCreateInfo, BufferUsage,
        allocator::{SubbufferAllocator, SubbufferAllocatorCreateInfo},
    },
    command_buffer::{AutoCommandBufferBuilder, CommandBufferUsage, RenderPassBeginInfo},
    descriptor_set::WriteDescriptorSet,
    device::DeviceOwned,
    format::Format,
    image::{
        Image, ImageCreateInfo, ImageType, ImageUsage,
        view::{ImageView, ImageViewCreateInfo},
    },
    memory::allocator::{AllocationCreateInfo, MemoryTypeFilter, StandardMemoryAllocator},
    pipeline::{
        GraphicsPipeline, Pipeline, PipelineBindPoint, PipelineLayout,
        PipelineShaderStageCreateInfo,
        graphics::{
            GraphicsPipelineCreateInfo,
            color_blend::{ColorBlendAttachmentState, ColorBlendState},
            depth_stencil::{DepthState, DepthStencilState},
            input_assembly::InputAssemblyState,
            multisample::MultisampleState,
            rasterization::RasterizationState,
            vertex_input::{Vertex, VertexDefinition},
            viewport::{Viewport, ViewportState},
        },
        layout::PipelineDescriptorSetLayoutCreateInfo,
    },
    render_pass::{Framebuffer, FramebufferCreateInfo, RenderPass, Subpass},
    shader::EntryPoint,
    swapchain::{SwapchainCreateInfo, SwapchainPresentInfo, acquire_next_image},
    sync::{self, GpuFuture},
};
use winit::application::ApplicationHandler;
use winit::{
    event::{Event, WindowEvent},
    event_loop::EventLoop,
};

pub struct ApolloApp {}

impl ApolloApp {
    pub fn new() -> Self {
        let mut ctx = crate::render::context::RenderContext::init(&event_loop);

        let vertex_buffer = Buffer::from_iter(
            ctx.allocators.memory.clone(),
            BufferCreateInfo {
                usage: BufferUsage::VERTEX_BUFFER,
                ..Default::default()
            },
            AllocationCreateInfo {
                memory_type_filter: MemoryTypeFilter::PREFER_DEVICE
                    | MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
                ..Default::default()
            },
            POSITIONS,
        )
        .unwrap();

        let normals_buffer = Buffer::from_iter(
            ctx.allocators.memory.clone(),
            BufferCreateInfo {
                usage: BufferUsage::VERTEX_BUFFER,
                ..Default::default()
            },
            AllocationCreateInfo {
                memory_type_filter: MemoryTypeFilter::PREFER_DEVICE
                    | MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
                ..Default::default()
            },
            NORMALS,
        )
        .unwrap();

        let index_buffer = Buffer::from_iter(
            ctx.allocators.memory.clone(),
            BufferCreateInfo {
                usage: BufferUsage::INDEX_BUFFER,
                ..Default::default()
            },
            AllocationCreateInfo {
                memory_type_filter: MemoryTypeFilter::PREFER_DEVICE
                    | MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
                ..Default::default()
            },
            INDICES,
        )
        .unwrap();

        let uniform_buffer = SubbufferAllocator::new(
            ctx.allocators.memory.clone(),
            SubbufferAllocatorCreateInfo {
                buffer_usage: BufferUsage::UNIFORM_BUFFER,
                memory_type_filter: MemoryTypeFilter::PREFER_DEVICE
                    | MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
                ..Default::default()
            },
        );

        let render_pass = vulkano::single_pass_renderpass!(
            ctx.device.clone(),
            attachments: {
                color: {
                    format: ctx.swapchain.image_format(),
                    samples: 1,
                    load_op: Clear,
                    store_op: Store,
                },
                depth_stencil: {
                    format: Format::D16_UNORM,
                    samples: 1,
                    load_op: Clear,
                    store_op: DontCare,
                },
            },
            pass: {
                color: [color],
                depth_stencil: {depth_stencil},
            },
        )
        .unwrap();

        let vs = shader::vs::load(ctx.device.clone())
            .unwrap()
            .entry_point("main")
            .unwrap();
        let fs = shader::fs::load(ctx.device.clone())
            .unwrap()
            .entry_point("main")
            .unwrap();

        let (mut pipeline, mut framebuffers) = window_size_dependent_setup(
            ctx.allocators.memory.clone(),
            vs.clone(),
            fs.clone(),
            &ctx.images,
            render_pass.clone(),
        );
        let mut recreate_swapchain = false;
        let mut recreate_swapchain_timer: Option<Instant> = None;

        let mut previous_frame_end = Some(sync::now(ctx.device.clone()).boxed());
        // let rotation_start = Instant::now();

        let mut slider_value = 7.0;
        let mut ts = 0;
        let mut scale = 0.01;

        let mut view = Matrix4::look_at_rh(
            Point3::new(0.3, 0.3, 1.0),
            Point3::new(0.0, 0.0, 0.0),
            Vector3::new(0.0, -1.0, 0.0),
        );

        Self {}
    }
}

impl ApplicationHandler for ApolloApp {
    fn resumed(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {}

    fn window_event(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        window_id: winit::window::WindowId,
        event: winit::event::WindowEvent,
    ) {
        match event {
            Event::WindowEvent { event, .. } => {
                if ctx.gui.update(&event) {
                    return;
                }
                match event {
                    WindowEvent::Moved(_) => {
                        recreate_swapchain_timer = Some(Instant::now());
                    }
                    WindowEvent::CloseRequested => control_flow.set_exit(),
                    WindowEvent::Resized(_) => {
                        recreate_swapchain = true;
                        recreate_swapchain_timer = Some(Instant::now());
                    }
                    _ => {}
                }
            }
            Event::RedrawEventsCleared => {
                if let Some(rst) = recreate_swapchain_timer {
                    if rst.elapsed().as_millis() < 100 {
                        return;
                    }
                }

                ctx.gui.immediate_ui(|gui| {
                    let ctx = &gui.context();

                    egui::Window::new("hi")
                        .default_pos((20.0, 20.0))
                        .show(ctx, |ui| {
                            ui.label("new label");
                            ui.label(format!("ts: {ts}"));
                            ui.add(
                                egui::Slider::new(&mut slider_value, 0.0..=10.0)
                                    .step_by(0.1)
                                    .text("rotation"),
                            );
                            ui.add(
                                egui::Slider::new(&mut scale, 0.001..=0.1)
                                    .step_by(0.001)
                                    .text("scale"),
                            );
                            macro_rules! value {
                                ($path:expr, $text:literal) => {
                                    ui.add(
                                        egui::Slider::new($path, -4.0..=4.0)
                                            .step_by(0.005)
                                            .text($text),
                                    );
                                };
                            }

                            value!(&mut view.x.x, "x.x");
                            value!(&mut view.x.y, "x.y");
                            value!(&mut view.x.z, "x.z");
                            value!(&mut view.x.w, "x.w");

                            value!(&mut view.y.x, "y.x");
                            value!(&mut view.y.y, "y.y");
                            value!(&mut view.y.z, "y.z");
                            value!(&mut view.y.w, "y.w");

                            value!(&mut view.z.x, "z.x");
                            value!(&mut view.z.y, "z.y");
                            value!(&mut view.z.z, "z.z");
                            value!(&mut view.z.w, "z.w");

                            value!(&mut view.w.x, "w.x");
                            value!(&mut view.w.y, "w.y");
                            value!(&mut view.w.z, "w.z");
                            value!(&mut view.w.w, "w.w");

                            if ui.button("quit").clicked() {
                                control_flow.set_exit();
                            }
                        });
                });

                let image_extent: [u32; 2] = ctx.window.inner_size().into();

                if image_extent.contains(&0) {
                    return;
                }

                previous_frame_end.as_mut().unwrap().cleanup_finished();

                if recreate_swapchain {
                    log::info!("recreating swapchain ...");
                    let (new_swapchain, new_images) = ctx
                        .swapchain
                        .recreate(SwapchainCreateInfo {
                            image_extent,
                            ..ctx.swapchain.create_info()
                        })
                        .expect("failed to recreate swapchain");

                    ctx.swapchain = new_swapchain;
                    let (new_pipeline, new_framebuffers) = window_size_dependent_setup(
                        ctx.allocators.memory.clone(),
                        vs.clone(),
                        fs.clone(),
                        &new_images,
                        render_pass.clone(),
                    );
                    pipeline = new_pipeline;
                    framebuffers = new_framebuffers;
                    recreate_swapchain = false;
                    recreate_swapchain_timer = None;
                }

                log::info!("uniform buffer subbuffer");
                let uniform_buffer_subbuffer = {
                    // let elapsed = rotation_start.elapsed();
                    // let rotation = elapsed.as_secs() as f64 + elapsed.subsec_nanos() as f64 / 1_000_000_000.0;
                    let rotation = Matrix3::from_angle_y(Rad(slider_value as f32));

                    // note: this teapot was meant for OpenGL where the origin is at the lower left
                    //       instead the origin is at the upper left in Vulkan, so we reverse the Y axis
                    let aspect_ratio = ctx.swapchain.image_extent()[0] as f32
                        / ctx.swapchain.image_extent()[1] as f32;
                    let proj = cgmath::perspective(
                        Rad(std::f32::consts::FRAC_PI_2),
                        aspect_ratio,
                        0.01,
                        100.0,
                    );

                    let scale = Matrix4::from_scale(scale);

                    let uniform_data = shader::vs::Data {
                        world: Matrix4::from(rotation).into(),
                        view: (view * scale).into(),
                        proj: proj.into(),
                    };

                    let subbuffer = uniform_buffer.allocate_sized().unwrap();
                    *subbuffer.write().unwrap() = uniform_data;

                    subbuffer
                };

                let layout = pipeline.layout().set_layouts().get(0).unwrap();
                let set = PersistentDescriptorSet::new(
                    &ctx.allocators.descriptor_set,
                    layout.clone(),
                    [WriteDescriptorSet::buffer(0, uniform_buffer_subbuffer)],
                    [],
                )
                .unwrap();

                log::info!("next image");
                let (image_index, suboptimal, acquire_future) = match acquire_next_image(
                    ctx.swapchain.clone(),
                    None,
                )
                .map_err(Validated::unwrap)
                {
                    Ok(r) => r,
                    Err(VulkanError::OutOfDate) => {
                        recreate_swapchain = true;
                        return;
                    }
                    Err(e) => panic!("failed to acquire next image: {e}"),
                };

                if suboptimal {
                    recreate_swapchain = true;
                }

                log::info!("auto command");
                let mut builder = AutoCommandBufferBuilder::primary(
                    Arc::new(ctx.allocators.command_buffer),
                    ctx.queue.queue_family_index(),
                    CommandBufferUsage::OneTimeSubmit,
                )
                .unwrap();

                builder
                    .begin_render_pass(
                        RenderPassBeginInfo {
                            clear_values: vec![
                                Some([0.015, 0.015, 0.015, 1.0].into()),
                                Some(1f32.into()),
                            ],
                            ..RenderPassBeginInfo::framebuffer(
                                framebuffers[image_index as usize].clone(),
                            )
                        },
                        Default::default(),
                    )
                    .unwrap()
                    .bind_pipeline_graphics(pipeline.clone())
                    .unwrap()
                    .bind_descriptor_sets(
                        PipelineBindPoint::Graphics,
                        pipeline.layout().clone(),
                        0,
                        set,
                    )
                    .unwrap()
                    .bind_vertex_buffers(0, (vertex_buffer.clone(), normals_buffer.clone()))
                    .unwrap()
                    .bind_index_buffer(index_buffer.clone())
                    .unwrap()
                    .draw_indexed(index_buffer.len() as u32, 1, 0, 0, 0)
                    .unwrap()
                    .end_render_pass(Default::default())
                    .unwrap();
                let command_buffer = builder.build().unwrap();

                log::info!("future");
                let future = previous_frame_end
                    .take()
                    .unwrap()
                    .join(acquire_future)
                    .then_execute(ctx.queue.clone(), command_buffer)
                    .unwrap()
                    // .then_swapchain_present(
                    //     ctx.queue.clone(),
                    //     SwapchainPresentInfo::swapchain_image_index(
                    //         ctx.swapchain.clone(),
                    //         image_index,
                    //     ),
                    // )
                    .then_signal_fence_and_flush()
                    .expect("future ..");

                let image = &ctx.images[image_index as usize];
                log::info!("iv cf");
                let iv_create_info = ImageViewCreateInfo {
                    // format: vulkano::format::Format::B8G8R8A8_UNORM,
                    format: vulkano::format::Format::B8G8R8A8_SRGB,
                    ..ImageViewCreateInfo::from_image(image)
                };
                log::info!("image view");
                println!("hi");
                log::info!("image: {image:?}");
                let gui_iv = ImageView::new(image.clone(), iv_create_info).expect("gui image view");

                let now = Instant::now();

                log::info!("context draw future");
                let result = ctx
                    .gui
                    .draw_on_image(future, gui_iv)
                    .then_swapchain_present(
                        ctx.queue.clone(),
                        SwapchainPresentInfo::swapchain_image_index(
                            ctx.swapchain.clone(),
                            image_index,
                        ),
                    )
                    .then_signal_fence_and_flush();

                ts = now.elapsed().as_micros();

                log::info!("check result");
                match result.map_err(Validated::unwrap) {
                    Ok(future) => {
                        previous_frame_end = Some(future.boxed());
                    }
                    Err(VulkanError::OutOfDate) => {
                        log::warn!("out of data");
                        recreate_swapchain = true;
                        previous_frame_end = Some(sync::now(ctx.device.clone()).boxed());
                    }
                    Err(e) => {
                        log::error!("failed to flush future: {e} = {e:?}");
                        previous_frame_end = Some(sync::now(ctx.device.clone()).boxed());
                    }
                }
            }
            _ => (),
        }
    }
}
