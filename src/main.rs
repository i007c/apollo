use egui::Window;
use egui_winit_vulkano::{Gui, GuiConfig};
use glam::{Mat4, Vec3};
use model::{INDICES, NORMALS, Normal, POSITIONS, Position};
use std::sync::Arc;
use vulkano::{
    buffer::{
        Buffer, BufferCreateInfo, BufferUsage, Subbuffer,
        allocator::{SubbufferAllocator, SubbufferAllocatorCreateInfo},
    },
    command_buffer::{
        AutoCommandBufferBuilder, CommandBufferInheritanceInfo,
        CommandBufferUsage, RenderPassBeginInfo, SubpassBeginInfo,
        SubpassContents,
        allocator::{
            StandardCommandBufferAllocator,
            StandardCommandBufferAllocatorCreateInfo,
        },
    },
    descriptor_set::{
        DescriptorSet, WriteDescriptorSet,
        allocator::StandardDescriptorSetAllocator,
    },
    device::{Device, Queue},
    format::Format,
    image::{Image, ImageCreateInfo, ImageType, ImageUsage, view::ImageView},
    memory::allocator::{
        AllocationCreateInfo, MemoryTypeFilter, StandardMemoryAllocator,
    },
    pipeline::{
        DynamicState, GraphicsPipeline, Pipeline, PipelineBindPoint,
        PipelineLayout, PipelineShaderStageCreateInfo,
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
    sync::GpuFuture,
};
use vulkano_util::{
    context::{VulkanoConfig, VulkanoContext},
    window::{VulkanoWindows, WindowDescriptor},
};
use winit::{
    application::ApplicationHandler, dpi::PhysicalSize, error::EventLoopError,
    event::WindowEvent, event_loop::EventLoop,
};

mod logger;
mod model;
mod shader;
mod utils;

pub struct App {
    context: VulkanoContext,
    windows: VulkanoWindows,
    gui_pipeline: Option<SimpleGuiPipeline>,
    gui: Option<Gui>,
}

impl Default for App {
    fn default() -> Self {
        // Vulkano context
        let context = VulkanoContext::new(VulkanoConfig::default());

        // Vulkano windows
        let windows = VulkanoWindows::default();

        Self { context, windows, gui_pipeline: None, gui: None }
    }
}

impl ApplicationHandler for App {
    fn resumed(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        let mut des = WindowDescriptor::default();
        des.title = "00-team-test-app".to_string();
        self.windows.create_window(event_loop, &self.context, &des, |ci| {
            ci.image_format = vulkano::format::Format::B8G8R8A8_UNORM;
            ci.min_image_count = ci.min_image_count.max(2);
        });

        let window_size =
            self.windows.get_primary_window().unwrap().inner_size();

        let renderer = self.windows.get_primary_renderer_mut().unwrap();
        // Create out gui pipeline
        let gui_pipeline = SimpleGuiPipeline::new(
            window_size,
            renderer.swapchain_image_view().image().extent(),
            self.context.graphics_queue().clone(),
            renderer.swapchain_format(),
            self.context.memory_allocator(),
        );

        // Create gui subpass
        self.gui = Some(Gui::new_with_subpass(
            event_loop,
            self.windows.get_primary_renderer_mut().unwrap().surface(),
            self.windows.get_primary_renderer_mut().unwrap().graphics_queue(),
            gui_pipeline.gui_pass(),
            self.windows.get_primary_renderer_mut().unwrap().swapchain_format(),
            GuiConfig::default(),
        ));

        self.gui_pipeline = Some(gui_pipeline);
    }

    fn window_event(
        &mut self, event_loop: &winit::event_loop::ActiveEventLoop,
        window_id: winit::window::WindowId, event: WindowEvent,
    ) {
        let renderer = self.windows.get_renderer_mut(window_id).unwrap();

        let gui = self.gui.as_mut().unwrap();
        let gpipe = self.gui_pipeline.as_mut().unwrap();

        match event {
            WindowEvent::Resized(_) => {
                renderer.resize();
            }
            WindowEvent::ScaleFactorChanged { .. } => {
                renderer.resize();
            }
            WindowEvent::CloseRequested => {
                event_loop.exit();
            }
            WindowEvent::RedrawRequested => {
                // Set immediate UI in redraw here
                gui.immediate_ui(|gui| {
                    let ctx = gui.context();
                    egui::Window::new("Settings")
                        .default_open(false)
                        .scroll([true, true])
                        .constrain(true)
                        .show(&ctx, |ui| ctx.settings_ui(ui));
                    Window::new("Transparent Window")
                        .resizable(true)
                        .default_width(300.0)
                        .movable(true)
                        .show(&ctx, |ui| {
                            ui.label("hi");
                            ui.add(
                                egui::Slider::new(
                                    &mut gpipe.rotation,
                                    -4.0..=4.0,
                                )
                                .step_by(0.005)
                                .text("rotation"),
                            );
                            ui.add(
                                egui::Slider::new(
                                    &mut gpipe.scale,
                                    0.001..=0.1,
                                )
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
                                ($path:expr, $text:literal, $min:literal, $max:literal) => {
                                    ui.add(
                                        egui::Slider::new($path, $min..=$max)
                                            .step_by(0.005)
                                            .text($text),
                                    );
                                };
                            }

                            value!(&mut gpipe.translation.x, "tr.x", -100.0, 100.0);
                            value!(&mut gpipe.translation.y, "tr.y", -100.0, 100.0);
                            value!(&mut gpipe.translation.z, "tr.z", -100.0, 100.0);

                            value!(&mut gpipe.cam_eye.x, "eye.x");
                            value!(&mut gpipe.cam_eye.y, "eye.y");
                            value!(&mut gpipe.cam_eye.z, "eye.z");

                            value!(&mut gpipe.cam_center.x, "center.x");
                            value!(&mut gpipe.cam_center.y, "center.y");
                            value!(&mut gpipe.cam_center.z, "center.z");

                            value!(&mut gpipe.cam_up.x, "up.x");
                            value!(&mut gpipe.cam_up.y, "up.y");
                            value!(&mut gpipe.cam_up.z, "up.z");

                            // value!(&mut gpipe.view.y_axis.x, "y.x");
                            // value!(&mut gpipe.view.y_axis.y, "y.y");
                            // value!(&mut gpipe.view.y_axis.z, "y.z");
                            // value!(&mut gpipe.view.y_axis.w, "y.w");
                            //
                            // value!(&mut gpipe.view.z_axis.x, "z.x");
                            // value!(&mut gpipe.view.z_axis.y, "z.y");
                            // value!(&mut gpipe.view.z_axis.z, "z.z");
                            // value!(&mut gpipe.view.z_axis.w, "z.w");
                            //
                            // value!(&mut gpipe.view.w_axis.x, "w.x");
                            // value!(&mut gpipe.view.w_axis.y, "w.y");
                            // value!(&mut gpipe.view.w_axis.z, "w.z");
                            // value!(&mut gpipe.view.w_axis.w, "w.w");

                            if ui.button("reset a").clicked() {
                                gpipe.scale = 0.01;
                                gpipe.rotation = 1.0;
                                gpipe.cam_eye = Vec3::new(0.3, 0.3, 1.0);
                                gpipe.cam_center = Vec3::new(0.0, 0.0, 0.0);
                                gpipe.cam_up = Vec3::new(0.0, -1.0, 0.0);
                            }

                            // ui.add(
                            //     egui::Slider::new(
                            //         &mut gpipe.rotation_b,
                            //         -4.0..=4.0,
                            //     )
                            //     .step_by(0.005)
                            //     .text("rotation_b"),
                            // );
                            // ui.add(
                            //     egui::Slider::new(
                            //         &mut gpipe.scale_b,
                            //         0.001..=0.1,
                            //     )
                            //     .step_by(0.001)
                            //     .text("scale_b"),
                            // );
                            //
                            // value!(&mut gpipe.view_b.x_axis.x, "x.x");
                            // value!(&mut gpipe.view_b.x_axis.y, "x.y");
                            // value!(&mut gpipe.view_b.x_axis.z, "x.z");
                            // value!(&mut gpipe.view_b.x_axis.w, "x.w");
                            //
                            // value!(&mut gpipe.view_b.y_axis.x, "y.x");
                            // value!(&mut gpipe.view_b.y_axis.y, "y.y");
                            // value!(&mut gpipe.view_b.y_axis.z, "y.z");
                            // value!(&mut gpipe.view_b.y_axis.w, "y.w");
                            //
                            // value!(&mut gpipe.view_b.z_axis.x, "z.x");
                            // value!(&mut gpipe.view_b.z_axis.y, "z.y");
                            // value!(&mut gpipe.view_b.z_axis.z, "z.z");
                            // value!(&mut gpipe.view_b.z_axis.w, "z.w");
                            //
                            // value!(&mut gpipe.view_b.w_axis.x, "w.x");
                            // value!(&mut gpipe.view_b.w_axis.y, "w.y");
                            // value!(&mut gpipe.view_b.w_axis.z, "w.z");
                            // value!(&mut gpipe.view_b.w_axis.w, "w.w");
                            //
                            // if ui.button("reset b").clicked() {
                            //     gpipe.scale_b = 0.01;
                            //     gpipe.rotation_b = 1.0;
                            //     gpipe.view_b = Mat4::look_at_rh(
                            //         Vec3::new(0.3, 0.3, 1.0),
                            //         Vec3::new(0.0, 0.0, 0.0),
                            //         Vec3::new(0.0, -1.0, 0.0),
                            //     );
                            // }
                        });
                });

                // Acquire swapchain future
                match renderer
                    .acquire(Some(std::time::Duration::from_millis(10)), |_| {})
                {
                    Ok(future) => {
                        // Render gui
                        let after_future = gpipe.render(
                            future,
                            renderer.swapchain_image_view(),
                            gui,
                        );

                        // Present swapchain
                        renderer.present(after_future, true);
                    }
                    Err(vulkano::VulkanError::OutOfDate) => {
                        renderer.resize();
                    }
                    Err(e) => {
                        panic!("Failed to acquire swapchain future: {}", e)
                    }
                };
            }
            _ => (),
        }

        if window_id == renderer.window().id() {
            // Update Egui integration so the UI works!
            let _pass_events_to_game = !gui.update(&event);
        }
    }

    fn about_to_wait(
        &mut self, _event_loop: &winit::event_loop::ActiveEventLoop,
    ) {
        let renderer = self.windows.get_primary_renderer().unwrap();

        renderer.window().request_redraw();
    }
}

pub fn main() -> Result<(), EventLoopError> {
    log::set_logger(&logger::MasterLogger).expect("log failed");
    log::set_max_level(log::LevelFilter::Debug);

    let event_loop = EventLoop::new().unwrap();

    let mut app = App::default();

    event_loop.run_app(&mut app)
}

struct SimpleGuiPipeline {
    queue: Arc<Queue>,
    render_pass: Arc<RenderPass>,
    pipeline: Arc<GraphicsPipeline>,
    subpass: Subpass,
    // vertex_buffer: Subbuffer<[MyVertex]>,
    command_buffer_allocator: Arc<StandardCommandBufferAllocator>,
    descriptor_set_allocator: Arc<StandardDescriptorSetAllocator>,
    vertex_buffer: Subbuffer<[Position]>,
    normals_buffer: Subbuffer<[Normal]>,
    index_buffer: Subbuffer<[u16]>,
    uniform_buffer_allocator: SubbufferAllocator,
    depth_buffer: Arc<ImageView>,
    scale: f32,
    rotation: f32,
    translation: Vec3,
    cam_eye: Vec3,
    cam_center: Vec3,
    cam_up: Vec3,
    // view: Mat4,
    // scale_b: f32,
    // rotation_b: f32,
    // view_b: Mat4,
}

impl SimpleGuiPipeline {
    pub fn new(
        window_size: PhysicalSize<u32>, image_extent: [u32; 3],
        queue: Arc<Queue>, image_format: vulkano::format::Format,
        allocator: &Arc<StandardMemoryAllocator>,
    ) -> Self {
        let render_pass =
            Self::create_render_pass(queue.device().clone(), image_format);
        let (pipeline, subpass) = Self::create_pipeline(
            window_size,
            queue.device().clone(),
            render_pass.clone(),
        );

        // Create an allocator for command-buffer data
        let command_buffer_allocator = StandardCommandBufferAllocator::new(
            queue.device().clone(),
            StandardCommandBufferAllocatorCreateInfo {
                secondary_buffer_count: 32,
                ..Default::default()
            },
        )
        .into();

        let descriptor_set_allocator =
            Arc::new(StandardDescriptorSetAllocator::new(
                queue.device().clone(),
                Default::default(),
            ));

        let vertex_buffer = Buffer::from_iter(
            allocator.clone(),
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
            allocator.clone(),
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
            allocator.clone(),
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

        // let vertex_buffer = Buffer::from_iter(
        //     allocator.clone(),
        //     BufferCreateInfo {
        //         usage: BufferUsage::VERTEX_BUFFER,
        //         ..Default::default()
        //     },
        //     AllocationCreateInfo {
        //         memory_type_filter: MemoryTypeFilter::PREFER_DEVICE
        //             | MemoryTypeFilter::HOST_RANDOM_ACCESS,
        //         ..Default::default()
        //     },
        //     [
        //         MyVertex {
        //             position: [-0.5, -0.25],
        //             color: [1.0, 0.0, 0.0, 1.0],
        //         },
        //         MyVertex { position: [0.0, 0.5], color: [0.0, 1.0, 0.0, 1.0] },
        //         MyVertex {
        //             position: [0.25, -0.1],
        //             color: [0.0, 0.0, 1.0, 1.0],
        //         },
        //     ],
        // )
        // .unwrap();

        let uniform_buffer_allocator = SubbufferAllocator::new(
            allocator.clone(),
            SubbufferAllocatorCreateInfo {
                buffer_usage: BufferUsage::UNIFORM_BUFFER,
                memory_type_filter: MemoryTypeFilter::PREFER_DEVICE
                    | MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
                ..Default::default()
            },
        );

        let depth_buffer = ImageView::new_default(
            Image::new(
                allocator.clone(),
                ImageCreateInfo {
                    image_type: ImageType::Dim2d,
                    format: Format::D16_UNORM,
                    extent: image_extent,
                    usage: ImageUsage::DEPTH_STENCIL_ATTACHMENT
                        | ImageUsage::TRANSIENT_ATTACHMENT,
                    ..Default::default()
                },
                AllocationCreateInfo::default(),
            )
            .unwrap(),
        )
        .unwrap();

        Self {
            queue,
            render_pass,
            pipeline,
            subpass,
            descriptor_set_allocator,
            vertex_buffer,
            normals_buffer,
            index_buffer,
            uniform_buffer_allocator,
            command_buffer_allocator,
            depth_buffer,
            rotation: 1.0,
            scale: 0.01,
            translation: Vec3::new(0.0, 0.0, 0.0),
            cam_eye: Vec3::new(0.3, 0.3, 1.0),
            cam_center: Vec3::new(0.0, 0.0, 0.0),
            cam_up: Vec3::new(0.0, -1.0, 0.0),
            // rotation_b: 1.0,
            // scale_b: 0.01,
            // view_b: Mat4::look_at_rh(
            //     Vec3::new(0.3, 0.3, 1.0),
            //     Vec3::new(0.0, 0.0, 0.0),
            //     Vec3::new(0.0, -1.0, 0.0),
            // ),
            // view_b: Mat4::look_at_rh(
            //     Vec3::new(2.0, 2.0, 2.0),
            //     Vec3::new(0.0, 0.0, 0.0),
            //     Vec3::new(0.0, -1.0, 0.0),
            // ),
        }
    }

    fn create_render_pass(
        device: Arc<Device>, format: Format,
    ) -> Arc<RenderPass> {
        // attachments: {
        //     color: {
        //         format: format,
        //         samples: SampleCount::Sample1,
        //         load_op: Clear,
        //         store_op: Store,
        //     }
        // },
        // passes: [
        //     { color: [color], depth_stencil: {}, input: [] }, // Draw what you want on this pass
        //     { color: [color], depth_stencil: {}, input: [] } // Gui render pass
        // ],
        vulkano::ordered_passes_renderpass!(
            device,
            attachments: {
                color: {
                    format: format,
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
            passes: [
                { color: [color], depth_stencil: {depth_stencil}, input: [] },
                { color: [color], depth_stencil: {}, input: [] }
            ],
        )
        .unwrap()
    }

    fn gui_pass(&self) -> Subpass {
        Subpass::from(self.render_pass.clone(), 1).unwrap()
    }

    fn create_pipeline(
        _window_size: PhysicalSize<u32>, device: Arc<Device>,
        render_pass: Arc<RenderPass>,
    ) -> (Arc<GraphicsPipeline>, Subpass) {
        let vs = crate::shader::vs::load(device.clone())
            .expect("failed to create shader module")
            .entry_point("main")
            .unwrap();
        let fs = crate::shader::fs::load(device.clone())
            .expect("failed to create shader module")
            .entry_point("main")
            .unwrap();

        // let vertex_input_state =
        //     MyVertex::per_vertex().definition(&vs).unwrap();
        let vertex_input_state = [Position::per_vertex(), Normal::per_vertex()]
            .definition(&vs)
            .unwrap();
        let stages = [
            PipelineShaderStageCreateInfo::new(vs),
            PipelineShaderStageCreateInfo::new(fs),
        ];

        let layout = PipelineLayout::new(
            device.clone(),
            PipelineDescriptorSetLayoutCreateInfo::from_stages(&stages)
                .into_pipeline_layout_create_info(device.clone())
                .unwrap(),
        )
        .unwrap();

        let subpass = Subpass::from(render_pass, 0).unwrap();
        (
            GraphicsPipeline::new(
                device,
                None,
                GraphicsPipelineCreateInfo {
                    stages: stages.into_iter().collect(),
                    vertex_input_state: Some(vertex_input_state),
                    input_assembly_state: Some(InputAssemblyState::default()),
                    viewport_state: Some(ViewportState::default()),
                    // viewport_state: Some(ViewportState {
                    //     viewports: [Viewport {
                    //         offset: [0.0, 0.0],
                    //         extent: window_size.into(),
                    //         depth_range: 0.0..=1.0,
                    //     }]
                    //     .into_iter()
                    //     .collect(),
                    //     ..Default::default()
                    // }),
                    rasterization_state: Some(RasterizationState::default()),
                    multisample_state: Some(MultisampleState::default()),
                    color_blend_state: Some(
                        ColorBlendState::with_attachment_states(
                            subpass.num_color_attachments(),
                            ColorBlendAttachmentState::default(),
                        ),
                    ),
                    dynamic_state: [DynamicState::Viewport]
                        .into_iter()
                        .collect(),
                    subpass: Some(subpass.clone().into()),

                    depth_stencil_state: Some(DepthStencilState {
                        depth: Some(DepthState::simple()),
                        ..Default::default()
                    }),

                    ..GraphicsPipelineCreateInfo::layout(layout)
                },
            )
            .unwrap(),
            subpass,
        )
    }

    pub fn render(
        &mut self, before_future: Box<dyn GpuFuture>, image: Arc<ImageView>,
        gui: &mut Gui,
    ) -> Box<dyn GpuFuture> {
        let mut builder = AutoCommandBufferBuilder::primary(
            self.command_buffer_allocator.clone(),
            self.queue.queue_family_index(),
            CommandBufferUsage::OneTimeSubmit,
        )
        .unwrap();

        let dimensions = image.image().extent();

        let framebuffer = Framebuffer::new(
            self.render_pass.clone(),
            FramebufferCreateInfo {
                // attachments: vec![image],
                attachments: vec![image, self.depth_buffer.clone()],
                ..Default::default()
            },
        )
        .unwrap();

        // let framebuffers = images
        //     .iter()
        //     .map(|image| {
        //         let view = ImageView::new_default(image.clone()).unwrap();
        //
        //         Framebuffer::new(
        //             render_pass.clone(),
        //             FramebufferCreateInfo {
        //                 attachments: vec![view, depth_buffer.clone()],
        //                 ..Default::default()
        //             },
        //         )
        //         .unwrap()
        //     })
        //     .collect::<Vec<_>>();

        // Begin render pipeline commands
        builder
            .begin_render_pass(
                RenderPassBeginInfo {
                    clear_values: vec![
                        Some([0.0, 0.0, 0.0, 1.0].into()),
                        Some(1f32.into()),
                    ],
                    ..RenderPassBeginInfo::framebuffer(framebuffer)
                },
                SubpassBeginInfo {
                    contents: SubpassContents::SecondaryCommandBuffers,
                    ..Default::default()
                },
            )
            .unwrap();

        // Render first draw pass
        let mut secondary_builder = AutoCommandBufferBuilder::secondary(
            self.command_buffer_allocator.clone(),
            self.queue.queue_family_index(),
            CommandBufferUsage::MultipleSubmit,
            CommandBufferInheritanceInfo {
                render_pass: Some(self.subpass.clone().into()),
                ..Default::default()
            },
        )
        .unwrap();
        secondary_builder
            .bind_pipeline_graphics(self.pipeline.clone())
            .unwrap()
            .set_viewport(
                0,
                [Viewport {
                    offset: [0.0, 0.0],
                    extent: [dimensions[0] as f32, dimensions[1] as f32],
                    depth_range: 0.0..=1.0,
                }]
                .into_iter()
                .collect(),
            )
            .unwrap();

        secondary_builder
            .bind_vertex_buffers(
                0,
                (self.vertex_buffer.clone(), self.normals_buffer.clone()),
            )
            .unwrap()
            .bind_index_buffer(self.index_buffer.clone())
            .unwrap();
        //     .bind_vertex_buffers(0, self.vertex_buffer.clone())
        //     .unwrap();
        // unsafe {
        //     secondary_builder
        //         .draw(self.vertex_buffer.len() as u32, 1, 0, 0)
        //         .unwrap();
        // }
        //
        let uniform_buffer_a = {
            // let elapsed = rcx.rotation_start.elapsed();
            // let rotation = elapsed.as_secs() as f64 + elapsed.subsec_nanos() as f64 / 1_000_000_000.0;
            // let rotation = Mat3::from_rotation_y(self.rotation as f32);

            // NOTE: This teapot was meant for OpenGL where the origin is at the lower left
            // instead the origin is at the upper left in Vulkan, so we reverse the Y axis.
            let aspect_ratio = dimensions[0] as f32 / dimensions[1] as f32;

            let proj = Mat4::perspective_rh_gl(
                std::f32::consts::FRAC_PI_2,
                aspect_ratio,
                0.01,
                100.0,
            );

            let model = Mat4::from_scale(Vec3::splat(self.scale))
                * Mat4::from_rotation_y(self.rotation)
                * Mat4::from_translation(self.translation);

            // let scale = Mat4::from_scale(Vec3::splat(self.scale));

            let view =
                Mat4::look_at_rh(self.cam_eye, self.cam_center, self.cam_up);
            // let view = Mat4::from_translation(self.cam_eye);
            let uniform_data = crate::shader::vs::Data {
                model: model.to_cols_array_2d(),
                view: view.to_cols_array_2d(),
                proj: proj.to_cols_array_2d(),
            };

            let buffer =
                self.uniform_buffer_allocator.allocate_sized().unwrap();
            *buffer.write().unwrap() = uniform_data;

            buffer
        };
        let layout = &self.pipeline.layout().set_layouts()[0];
        let descriptor_set_a = DescriptorSet::new(
            self.descriptor_set_allocator.clone(),
            layout.clone(),
            [WriteDescriptorSet::buffer(0, uniform_buffer_a)],
            [],
        )
        .unwrap();

        // let uniform_buffer_b = {
        //     // let elapsed = rcx.rotation_start.elapsed();
        //     // let rotation = elapsed.as_secs() as f64 + elapsed.subsec_nanos() as f64 / 1_000_000_000.0;
        //     let rotation = Mat3::from_rotation_y(self.rotation_b as f32);
        //
        //     // NOTE: This teapot was meant for OpenGL where the origin is at the lower left
        //     // instead the origin is at the upper left in Vulkan, so we reverse the Y axis.
        //     let aspect_ratio = dimensions[0] as f32 / dimensions[1] as f32;
        //
        //     let proj = Mat4::perspective_rh_gl(
        //         std::f32::consts::FRAC_PI_2,
        //         aspect_ratio,
        //         0.01,
        //         100.0,
        //     );
        //
        //     let scale = Mat4::from_scale(Vec3::splat(self.scale_b));
        //
        //     let uniform_data = crate::shader::vs::Data {
        //         model_rotate: Mat4::from_mat3(rotation).to_cols_array_2d(),
        //         view: (self.view_b * scale).to_cols_array_2d(),
        //         proj: proj.to_cols_array_2d(),
        //     };
        //
        //     let buffer =
        //         self.uniform_buffer_allocator.allocate_sized().unwrap();
        //     *buffer.write().unwrap() = uniform_data;
        //
        //     buffer
        // };

        // let descriptor_set_b = DescriptorSet::new(
        //     self.descriptor_set_allocator.clone(),
        //     layout.clone(),
        //     [WriteDescriptorSet::buffer(0, uniform_buffer_b)],
        //     [],
        // )
        // .unwrap();

        secondary_builder
            .bind_descriptor_sets(
                PipelineBindPoint::Graphics,
                self.pipeline.layout().clone(),
                0,
                descriptor_set_a,
            )
            .unwrap();
        unsafe {
            secondary_builder.draw_indexed(
                self.index_buffer.len() as u32,
                1,
                0,
                0,
                0,
            )
        }
        .unwrap();

        // secondary_builder
        //     .bind_descriptor_sets(
        //         PipelineBindPoint::Graphics,
        //         self.pipeline.layout().clone(),
        //         0,
        //         descriptor_set_b,
        //     )
        //     .unwrap();
        // unsafe {
        //     secondary_builder.draw_indexed(
        //         self.index_buffer.len() as u32,
        //         1,
        //         0,
        //         0,
        //         0,
        //     )
        // }
        // .unwrap();

        let cb = secondary_builder.build().unwrap();
        builder.execute_commands(cb).unwrap();

        // Move on to next subpass for gui
        builder
            .next_subpass(
                Default::default(),
                SubpassBeginInfo {
                    contents: SubpassContents::SecondaryCommandBuffers,
                    ..Default::default()
                },
            )
            .unwrap();
        // Draw gui on subpass
        let cb = gui.draw_on_subpass_image([dimensions[0], dimensions[1]]);
        builder.execute_commands(cb).unwrap();

        // Last end render pass
        builder.end_render_pass(Default::default()).unwrap();
        let command_buffer = builder.build().unwrap();
        let after_future = before_future
            .then_execute(self.queue.clone(), command_buffer)
            .unwrap();

        after_future.boxed()
    }
}
