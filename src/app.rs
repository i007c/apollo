use crate::model::{INDICES, NORMALS, Normal, POSITIONS, Position};
use crate::render::context::RenderContext;
use egui_winit_vulkano::egui;
use glam::{
    Mat4,
    f32::{Mat3, Vec3},
};
use std::sync::Arc;
use std::time::Instant;
use vulkano::VulkanLibrary;
use vulkano::buffer::Subbuffer;
use vulkano::buffer::allocator::SubbufferAllocator;
use vulkano::command_buffer::allocator::StandardCommandBufferAllocator;
use vulkano::descriptor_set::allocator::StandardDescriptorSetAllocator;
use vulkano::device::{DeviceExtensions, QueueFlags};
use vulkano::image::view::{ImageView, ImageViewCreateInfo};
use vulkano::instance::{InstanceCreateFlags, InstanceCreateInfo};
use vulkano::swapchain::Surface;
use vulkano::{
    Validated, VulkanError,
    buffer::{
        Buffer, BufferCreateInfo, BufferUsage,
        allocator::SubbufferAllocatorCreateInfo,
    },
    command_buffer::{
        AutoCommandBufferBuilder, CommandBufferUsage, RenderPassBeginInfo,
    },
    descriptor_set::{DescriptorSet, WriteDescriptorSet},
    device::{
        Device, DeviceCreateInfo, Queue, QueueCreateInfo,
        physical::PhysicalDeviceType,
    },
    instance::Instance,
    memory::allocator::{
        AllocationCreateInfo, MemoryTypeFilter, StandardMemoryAllocator,
    },
    pipeline::{Pipeline, PipelineBindPoint},
    swapchain::{
        SwapchainCreateInfo, SwapchainPresentInfo, acquire_next_image,
    },
    sync::{self, GpuFuture},
};
use winit::application::ApplicationHandler;
use winit::event::WindowEvent;
use winit::event_loop::EventLoop;
use winit::{event_loop::ActiveEventLoop, window::WindowId};

pub struct ApolloApp {
    pub instance: Arc<Instance>,
    pub device: Arc<Device>,
    pub queue: Arc<Queue>,
    pub memory_allocator: Arc<StandardMemoryAllocator>,
    pub descriptor_set_allocator: Arc<StandardDescriptorSetAllocator>,
    pub command_buffer_allocator: Arc<StandardCommandBufferAllocator>,
    pub vertex_buffer: Subbuffer<[Position]>,
    pub normals_buffer: Subbuffer<[Normal]>,
    pub index_buffer: Subbuffer<[u16]>,
    pub uniform_buffer_allocator: SubbufferAllocator,
    pub rcx: Option<RenderContext>,

    // rctx: Option<RenderContext>,
    // recreate_swapchain: bool,
    // recreate_swapchain_timer: Option<Instant>,
    slider_value: f64,
    ts: u128,
    scale: f32,
    // view: Matrix4<f32>,
    view: Mat4,
    // previous_frame_end: Option<Box<dyn GpuFuture>>,
    exit: bool,
}

impl ApolloApp {
    pub fn new(event_loop: &EventLoop<()>) -> Self {
        let library = VulkanLibrary::new().unwrap();
        let required_extensions =
            Surface::required_extensions(event_loop).unwrap();
        let instance = Instance::new(
            library,
            InstanceCreateInfo {
                flags: InstanceCreateFlags::ENUMERATE_PORTABILITY,
                enabled_extensions: required_extensions,
                ..Default::default()
            },
        )
        .unwrap();

        let device_extensions = DeviceExtensions {
            khr_swapchain: true,
            ..DeviceExtensions::empty()
        };
        let (physical_device, queue_family_index) = instance
            .enumerate_physical_devices()
            .unwrap()
            .filter(|p| p.supported_extensions().contains(&device_extensions))
            .filter_map(|p| {
                p.queue_family_properties()
                    .iter()
                    .enumerate()
                    .position(|(i, q)| {
                        q.queue_flags.intersects(QueueFlags::GRAPHICS)
                            && p.presentation_support(i as u32, event_loop)
                                .unwrap()
                    })
                    .map(|i| (p, i as u32))
            })
            .min_by_key(|(p, _)| match p.properties().device_type {
                PhysicalDeviceType::DiscreteGpu => 0,
                PhysicalDeviceType::IntegratedGpu => 1,
                PhysicalDeviceType::VirtualGpu => 2,
                PhysicalDeviceType::Cpu => 3,
                PhysicalDeviceType::Other => 4,
                _ => 5,
            })
            .unwrap();

        log::info!(
            "Using device: {} (type: {:?})",
            physical_device.properties().device_name,
            physical_device.properties().device_type,
        );

        let (device, mut queues) = Device::new(
            physical_device,
            DeviceCreateInfo {
                enabled_extensions: device_extensions,
                queue_create_infos: vec![QueueCreateInfo {
                    queue_family_index,
                    ..Default::default()
                }],
                ..Default::default()
            },
        )
        .unwrap();

        let queue = queues.next().unwrap();

        let memory_allocator =
            Arc::new(StandardMemoryAllocator::new_default(device.clone()));
        let descriptor_set_allocator =
            Arc::new(StandardDescriptorSetAllocator::new(
                device.clone(),
                Default::default(),
            ));
        let command_buffer_allocator =
            Arc::new(StandardCommandBufferAllocator::new(
                device.clone(),
                Default::default(),
            ));

        let vertex_buffer = Buffer::from_iter(
            memory_allocator.clone(),
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
            memory_allocator.clone(),
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
            memory_allocator.clone(),
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

        let uniform_buffer_allocator = SubbufferAllocator::new(
            memory_allocator.clone(),
            SubbufferAllocatorCreateInfo {
                buffer_usage: BufferUsage::UNIFORM_BUFFER,
                memory_type_filter: MemoryTypeFilter::PREFER_DEVICE
                    | MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
                ..Default::default()
            },
        );

        Self {
            instance,
            device,
            queue,
            memory_allocator,
            descriptor_set_allocator,
            command_buffer_allocator,
            vertex_buffer,
            normals_buffer,
            index_buffer,
            uniform_buffer_allocator,
            rcx: None,
            slider_value: 7.0,
            ts: 0,
            scale: 0.01,
            // view: Matrix4::look_at_rh(
            //     Point3::new(0.3, 0.3, 1.0),
            //     Point3::new(0.0, 0.0, 0.0),
            //     Vector3::new(0.0, -1.0, 0.0),
            // ),
            view: Mat4::look_at_rh(
                Vec3::new(0.3, 0.3, 1.0),
                Vec3::new(0.0, 0.0, 0.0),
                Vec3::new(0.0, -1.0, 0.0),
            ),
            exit: false,
        }

        // Self {
        //     recreate_swapchain: false,
        //     recreate_swapchain_timer: None,
        //     previous_frame_end: None,
        // }
    }

    fn redraw(&mut self) {
        let rcx = self.rcx.as_mut().unwrap();
        let window_size = rcx.window.inner_size();

        if window_size.width == 0 || window_size.height == 0 {
            return;
        }

        rcx.gui.immediate_ui(|gui| {
            let ctx = &gui.context();

            egui::Window::new("hi").default_pos((20.0, 20.0)).show(ctx, |ui| {
                ui.label("new label");
                ui.label(format!("ts: {}", self.ts));
                ui.add(
                    egui::Slider::new(&mut self.slider_value, 0.0..=10.0)
                        .step_by(0.1)
                        .text("rotation"),
                );
                ui.add(
                    egui::Slider::new(&mut self.scale, 0.001..=0.1)
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

                value!(&mut self.view.x_axis.x, "x.x");
                value!(&mut self.view.x_axis.y, "x.y");
                value!(&mut self.view.x_axis.z, "x.z");
                value!(&mut self.view.x_axis.w, "x.w");

                value!(&mut self.view.y_axis.x, "y.x");
                value!(&mut self.view.y_axis.y, "y.y");
                value!(&mut self.view.y_axis.z, "y.z");
                value!(&mut self.view.y_axis.w, "y.w");

                value!(&mut self.view.z_axis.x, "z.x");
                value!(&mut self.view.z_axis.y, "z.y");
                value!(&mut self.view.z_axis.z, "z.z");
                value!(&mut self.view.z_axis.w, "z.w");

                value!(&mut self.view.w_axis.x, "w.x");
                value!(&mut self.view.w_axis.y, "w.y");
                value!(&mut self.view.w_axis.z, "w.z");
                value!(&mut self.view.w_axis.w, "w.w");

                if ui.button("quit").clicked() {
                    self.exit = true;
                }
            });
        });

        rcx.previous_frame_end.as_mut().unwrap().cleanup_finished();

        if rcx.recreate_swapchain {
            let (new_swapchain, new_images) = rcx
                .swapchain
                .recreate(SwapchainCreateInfo {
                    image_extent: window_size.into(),
                    ..rcx.swapchain.create_info()
                })
                .expect("failed to recreate swapchain");

            rcx.swapchain = new_swapchain;
            (rcx.framebuffers, rcx.pipeline) =
                crate::utils::window_size_dependent_setup(
                    window_size,
                    &new_images,
                    &rcx.render_pass,
                    &self.memory_allocator,
                    &rcx.vs,
                    &rcx.fs,
                );
            rcx.recreate_swapchain = false;
            // rcx.images = new_images
            //     .into_iter()
            //     .map(|image| ImageView::new_default(image).unwrap())
            //     .collect::<Vec<_>>();
        }

        let uniform_buffer = {
            let elapsed = rcx.rotation_start.elapsed();
            let rotation = elapsed.as_secs() as f64
                + elapsed.subsec_nanos() as f64 / 1_000_000_000.0;
            let rotation = Mat3::from_rotation_y(rotation as f32);

            // NOTE: This teapot was meant for OpenGL where the origin is at the lower left
            // instead the origin is at the upper left in Vulkan, so we reverse the Y axis.
            let aspect_ratio = rcx.swapchain.image_extent()[0] as f32
                / rcx.swapchain.image_extent()[1] as f32;

            let proj = Mat4::perspective_rh_gl(
                std::f32::consts::FRAC_PI_2,
                aspect_ratio,
                0.01,
                100.0,
            );

            let scale = Mat4::from_scale(Vec3::splat(self.scale));

            let uniform_data = crate::shader::vs::Data {
                world: Mat4::from_mat3(rotation).to_cols_array_2d(),
                view: (self.view * scale).to_cols_array_2d(),
                proj: proj.to_cols_array_2d(),
            };

            let buffer =
                self.uniform_buffer_allocator.allocate_sized().unwrap();
            *buffer.write().unwrap() = uniform_data;

            buffer
        };

        let layout = &rcx.pipeline.layout().set_layouts()[0];
        let descriptor_set = DescriptorSet::new(
            self.descriptor_set_allocator.clone(),
            layout.clone(),
            [WriteDescriptorSet::buffer(0, uniform_buffer)],
            [],
        )
        .unwrap();

        let (image_index, suboptimal, acquire_future) =
            match acquire_next_image(rcx.swapchain.clone(), None)
                .map_err(Validated::unwrap)
            {
                Ok(r) => r,
                Err(VulkanError::OutOfDate) => {
                    rcx.recreate_swapchain = true;
                    return;
                }
                Err(e) => panic!("failed to acquire next image: {e}"),
            };

        if suboptimal {
            rcx.recreate_swapchain = true;
        }

        let mut builder = AutoCommandBufferBuilder::primary(
            self.command_buffer_allocator.clone(),
            self.queue.queue_family_index(),
            CommandBufferUsage::OneTimeSubmit,
        )
        .unwrap();

        builder
            .begin_render_pass(
                RenderPassBeginInfo {
                    clear_values: vec![
                        Some([0.0, 0.0, 1.0, 1.0].into()),
                        Some(1f32.into()),
                    ],
                    ..RenderPassBeginInfo::framebuffer(
                        rcx.framebuffers[image_index as usize].clone(),
                    )
                },
                Default::default(),
            )
            .unwrap()
            .bind_pipeline_graphics(rcx.pipeline.clone())
            .unwrap()
            .bind_descriptor_sets(
                PipelineBindPoint::Graphics,
                rcx.pipeline.layout().clone(),
                0,
                descriptor_set,
            )
            .unwrap()
            .bind_vertex_buffers(
                0,
                (self.vertex_buffer.clone(), self.normals_buffer.clone()),
            )
            .unwrap()
            .bind_index_buffer(self.index_buffer.clone())
            .unwrap();
        unsafe {
            builder.draw_indexed(self.index_buffer.len() as u32, 1, 0, 0, 0)
        }
        .unwrap();

        // let sacb = rcx.gui.draw_on_subpass_image(rcx.swapchain.image_extent());
        // builder.execute_commands(sacb).unwrap();

        builder.end_render_pass(Default::default()).unwrap();

        let command_buffer = builder.build().unwrap();
        let future =
            rcx.previous_frame_end.take().unwrap().join(acquire_future);

        // log::info!("after fut");
        // let after_fut = rcx
        //     .gui
        //     .draw_on_image(future, rcx.images[image_index as usize].clone());

        // log::info!("after fut res");
        let res = future
            .then_execute(self.queue.clone(), command_buffer)
            .unwrap()
            .then_swapchain_present(
                self.queue.clone(),
                // SwapchainPresentInfo::new(rcx.swapchain.clone(), image_index),
                SwapchainPresentInfo::swapchain_image_index(
                    rcx.swapchain.clone(),
                    image_index,
                ),
            )
            .then_signal_fence_and_flush();

        // ------------------- //
        // log::info!("images.len(): {} | {}", rcx.images.len(), image_index);
        // let image = &rcx.images[image_index as usize];
        // // let image = &rctx.images[0];
        // log::info!("iv cf");
        // let iv_create_info = ImageViewCreateInfo {
        //     format: vulkano::format::Format::B8G8R8A8_UNORM,
        //     // format: vulkano::format::Format::B8G8R8A8_,
        //     ..ImageViewCreateInfo::from_image(image)
        // };
        // log::info!("image view");
        // let gui_iv = ImageView::new(image.clone(), iv_create_info)
        //     .expect("gui image view");
        //
        // let now = Instant::now();

        // log::info!("context draw future");
        // let result = rcx
        //     .gui
        //     .draw_on_image(future, gui_iv)
        //     .then_swapchain_present(
        //         self.queue.clone(),
        //         SwapchainPresentInfo::swapchain_image_index(
        //             rcx.swapchain.clone(),
        //             image_index,
        //         ),
        //     )
        //     .then_signal_fence_and_flush();

        // ---------------------- //

        match res.map_err(Validated::unwrap) {
            Ok(future) => {
                // rcx.gui.draw_on_image(future, rcx.swapchain.image_v)
                rcx.previous_frame_end = Some(future.boxed());
            }
            Err(VulkanError::OutOfDate) => {
                rcx.recreate_swapchain = true;
                rcx.previous_frame_end =
                    Some(sync::now(self.device.clone()).boxed());
            }
            Err(e) => {
                println!("failed to flush future: {e}");
                rcx.previous_frame_end =
                    Some(sync::now(self.device.clone()).boxed());
            }
        }
    }
}

impl ApplicationHandler for ApolloApp {
    fn window_event(
        &mut self, event_loop: &ActiveEventLoop, _window_id: WindowId,
        event: WindowEvent,
    ) {
        let rcx = self.rcx.as_mut().unwrap();
        let _pass_events_to_game = !rcx.gui.update(&event);

        match event {
            WindowEvent::CloseRequested => {
                event_loop.exit();
            }
            WindowEvent::Resized(_) => {
                rcx.recreate_swapchain = true;
            }
            WindowEvent::RedrawRequested => {
                self.redraw();
            }
            _ => {}
        }
    }

    fn about_to_wait(&mut self, event_loop: &ActiveEventLoop) {
        if self.exit {
            event_loop.exit();
            return;
        }
        let rcx = self.rcx.as_mut().unwrap();
        rcx.window.request_redraw();
    }

    fn resumed(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
        self.rcx = Some(RenderContext::new(event_loop, &self));

        // log::info!("resumed");
        // let rctx = crate::render::context::RenderContext::init(&event_loop);
        // self.previous_frame_end = Some(sync::now(rctx.device.clone()).boxed());
        // self.rctx = Some(rctx);
    }
}
