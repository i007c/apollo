use std::sync::Arc;

use vulkano::{
    instance::{Instance, InstanceCreateFlags, InstanceCreateInfo},
    swapchain::Surface,
    VulkanLibrary,
};
use winit::{
    application::ApplicationHandler,
    event::WindowEvent,
    event_loop::{ActiveEventLoop, ControlFlow, EventLoop},
    window::{Theme, Window},
};

#[derive(Default)]
struct App {
    window: Option<Arc<Window>>,
}

impl ApplicationHandler for App {
    fn resumed(&mut self, event_loop: &ActiveEventLoop) {
        self.window = Some(Arc::new(
            event_loop
                .create_window(
                    Window::default_attributes()
                        .with_title("Apollo")
                        .with_theme(Some(Theme::Dark)),
                )
                .expect("create window"),
        ));
    }

    fn window_event(
        &mut self,
        event_loop: &ActiveEventLoop,
        _: winit::window::WindowId,
        event: WindowEvent,
    ) {
        match event {
            WindowEvent::CloseRequested => {
                event_loop.exit();
            }
            WindowEvent::RedrawRequested => {
                self.window.as_ref().unwrap().request_redraw();
            }
            _ => (),
        }
    }
}

fn main() {
    dotenvy::from_path(".env").expect("could not load .env");
    pretty_env_logger::init();

    let event_loop = EventLoop::new().expect("event loop");
    event_loop.set_control_flow(ControlFlow::Poll);
    let library = VulkanLibrary::new().expect("vulkan library");

    let required_extensions = Surface::required_extensions(&event_loop);

    let instance = Instance::new(
        library,
        InstanceCreateInfo {
            flags: InstanceCreateFlags::ENUMERATE_PORTABILITY,
            enabled_extensions: required_extensions,
            ..Default::default()
        },
    )
    .expect("instance creation");

    let mut app = App::default();
    event_loop.run_app(&mut app).expect("run app");

    // let window = event_loop.create_window(window_attributes);
}
