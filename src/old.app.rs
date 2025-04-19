fn about_to_wait(&mut self, event_loop: &winit::event_loop::ActiveEventLoop) {
    log::debug!("about to wait");
    self.redraw();
}

fn window_event(
    &mut self, event_loop: &winit::event_loop::ActiveEventLoop,
    _wid: winit::window::WindowId, event: winit::event::WindowEvent,
) {
    // log::info!("window event: {event:?}");
    let rctx = self.rctx.as_mut().unwrap();
    if rctx.gui.update(&event) {
        return;
    }
    match event {
        WindowEvent::Moved(_) => {
            self.recreate_swapchain_timer = Some(Instant::now());
        }
        WindowEvent::CloseRequested => {
            event_loop.exit();
        }
        WindowEvent::Resized(_) => {
            self.recreate_swapchain = true;
            self.recreate_swapchain_timer = Some(Instant::now());
        }
        WindowEvent::RedrawRequested => {
            log::info!("redraw");
            self.redraw();
        }
        _ => {}
    }

    // match event {
    //     Event::RedrawEventsCleared => {
    //     }
    //     _ => (),
    // }
}

pub fn redraw(&mut self) {
    let Some(rctx) = &mut self.rctx else { return };

    if let Some(rst) = self.recreate_swapchain_timer {
        if rst.elapsed().as_millis() < 100 {
            return;
        }
    }

    rctx.gui.immediate_ui(|gui| {
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

            value!(&mut self.view.x.x, "x.x");
            value!(&mut self.view.x.y, "x.y");
            value!(&mut self.view.x.z, "x.z");
            value!(&mut self.view.x.w, "x.w");

            value!(&mut self.view.y.x, "y.x");
            value!(&mut self.view.y.y, "y.y");
            value!(&mut self.view.y.z, "y.z");
            value!(&mut self.view.y.w, "y.w");

            value!(&mut self.view.z.x, "z.x");
            value!(&mut self.view.z.y, "z.y");
            value!(&mut self.view.z.z, "z.z");
            value!(&mut self.view.z.w, "z.w");

            value!(&mut self.view.w.x, "w.x");
            value!(&mut self.view.w.y, "w.y");
            value!(&mut self.view.w.z, "w.z");
            value!(&mut self.view.w.w, "w.w");

            if ui.button("quit").clicked() {
                self.exit = true;
            }
        });
    });

    let image_extent: [u32; 2] = rctx.window.inner_size().into();

    log::info!("image_extent: {image_extent:?}");
    if image_extent.contains(&0) {
        return;
    }

    self.previous_frame_end.as_mut().unwrap().cleanup_finished();

    if self.recreate_swapchain {
        log::warn!("recreating swapchain ...");
        let (new_swapchain, new_images) = rctx
            .swapchain
            .recreate(SwapchainCreateInfo {
                image_extent,
                ..rctx.swapchain.create_info()
            })
            .expect("failed to recreate swapchain");

        rctx.swapchain = new_swapchain;
        let (new_pipeline, new_framebuffers) =
            crate::utils::window_size_dependent_setup(
                rctx.allocators.memory.clone(),
                rctx.vs.clone(),
                rctx.fs.clone(),
                &new_images,
                rctx.render_pass.clone(),
            );
        rctx.images = new_images;
        rctx.pipeline = new_pipeline;
        rctx.framebuffers = new_framebuffers;
        self.recreate_swapchain = false;
        self.recreate_swapchain_timer = None;
    }

    log::info!("uniform buffer subbuffer");
    let uniform_buffer_subbuffer = {
        // let elapsed = rotation_start.elapsed();
        // let rotation = elapsed.as_secs() as f64 + elapsed.subsec_nanos() as f64 / 1_000_000_000.0;
        let rotation = Matrix3::from_angle_y(Rad(self.slider_value as f32));

        // note: this teapot was meant for OpenGL where the origin is at the lower left
        //       instead the origin is at the upper left in Vulkan, so we reverse the Y axis
        let aspect_ratio = rctx.swapchain.image_extent()[0] as f32
            / rctx.swapchain.image_extent()[1] as f32;
        let proj = cgmath::perspective(
            Rad(std::f32::consts::FRAC_PI_2),
            aspect_ratio,
            0.01,
            100.0,
        );

        let scale = Matrix4::from_scale(self.scale);

        let uniform_data = crate::shader::vs::Data {
            world: Matrix4::from(rotation).into(),
            view: (self.view * scale).into(),
            proj: proj.into(),
        };

        let subbuffer = rctx.uniform_buffer.allocate_sized().unwrap();
        *subbuffer.write().unwrap() = uniform_data;

        subbuffer
    };

    let allocators = Allocators::new(rctx.device.clone());

    let layout = rctx.pipeline.layout().set_layouts().get(0).unwrap();
    let set = DescriptorSet::new(
        Arc::new(allocators.descriptor_set),
        layout.clone(),
        [WriteDescriptorSet::buffer(0, uniform_buffer_subbuffer)],
        [],
    )
    .unwrap();

    log::info!("next image");
    let (image_index, suboptimal, acquire_future) =
        match acquire_next_image(rctx.swapchain.clone(), None)
            .map_err(Validated::unwrap)
        {
            Ok(r) => r,
            Err(VulkanError::OutOfDate) => {
                self.recreate_swapchain = true;
                return;
            }
            Err(e) => panic!("failed to acquire next image: {e}"),
        };

    if suboptimal {
        log::info!("is suboptimal");
        self.recreate_swapchain = true;
    }

    log::info!("auto command");
    let mut builder = AutoCommandBufferBuilder::primary(
        Arc::new(allocators.command_buffer),
        rctx.queue.queue_family_index(),
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
                    rctx.framebuffers[image_index as usize].clone(),
                )
            },
            Default::default(),
        )
        .expect("begin render pass")
        .bind_pipeline_graphics(rctx.pipeline.clone())
        .expect("bind_pipeline_graphics")
        .bind_descriptor_sets(
            PipelineBindPoint::Graphics,
            rctx.pipeline.layout().clone(),
            0,
            set,
        )
        .expect("bind descriptor_set")
        .bind_vertex_buffers(
            0,
            (rctx.vertex_buffer.clone(), rctx.normals_buffer.clone()),
        )
        .expect("bind_vertex_buffers")
        .bind_index_buffer(rctx.index_buffer.clone())
        .expect("bind_index_buffer");

    unsafe {
        builder
            .draw_indexed(rctx.index_buffer.len() as u32, 1, 0, 0, 0)
            .expect("draw indexed");
    }
    builder.end_render_pass(Default::default()).expect("end render pass");
    let command_buffer = builder.build().expect("cmd buff build");

    log::info!("future");
    let future = self
        .previous_frame_end
        .take()
        .expect("no prev frame")
        .join(acquire_future)
        .then_execute(rctx.queue.clone(), command_buffer)
        .expect("future then execute")
        .then_swapchain_present(
            rctx.queue.clone(),
            SwapchainPresentInfo::swapchain_image_index(
                rctx.swapchain.clone(),
                image_index,
            ),
        )
        .then_signal_fence_and_flush()
        .expect("future ..");

    log::info!("images.len(): {} | {}", rctx.images.len(), image_index);
    let image = &rctx.images[image_index as usize];
    // let image = &rctx.images[0];
    log::info!("iv cf");
    let iv_create_info = ImageViewCreateInfo {
        format: vulkano::format::Format::B8G8R8A8_UNORM,
        // format: vulkano::format::Format::B8G8R8A8_,
        ..ImageViewCreateInfo::from_image(image)
    };
    log::info!("image view");
    let gui_iv = ImageView::new(image.clone(), iv_create_info)
        .expect("gui image view");

    let now = Instant::now();

    log::info!("context draw future");
    let result = rctx
        .gui
        .draw_on_image(future, gui_iv)
        .then_swapchain_present(
            rctx.queue.clone(),
            SwapchainPresentInfo::swapchain_image_index(
                rctx.swapchain.clone(),
                image_index,
            ),
        )
        .then_signal_fence_and_flush();

    self.ts = now.elapsed().as_micros();

    log::info!("check result");
    match result.map_err(Validated::unwrap) {
        Ok(future) => {
            self.previous_frame_end = Some(future.boxed());
        }
        Err(VulkanError::OutOfDate) => {
            log::warn!("out of data");
            self.recreate_swapchain = true;
            self.previous_frame_end =
                Some(sync::now(rctx.device.clone()).boxed());
        }
        Err(e) => {
            log::error!("failed to flush future: {e} = {e:?}");
            self.previous_frame_end =
                Some(sync::now(rctx.device.clone()).boxed());
        }
    }
}
