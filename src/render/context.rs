use std::sync::Arc;

use egui_winit_vulkano::{Gui, GuiConfig};
use vulkano::{
    device::{Device, Queue},
    image::Image,
    instance::Instance,
    pipeline::graphics::viewport::Viewport,
    swapchain::{Surface, Swapchain},
};
use winit::{
    event_loop::ActiveEventLoop,
    window::{Window, WindowAttributes},
};

use crate::object::{allocators::Allocators, device, instance, swapchain};

pub struct RenderContext {
    instance: Arc<Instance>,
    pub window: Arc<Window>,
    pub viewport: Viewport,
    pub device: Arc<Device>,
    pub queue: Arc<Queue>,
    pub allocators: Allocators,
    pub swapchain: Arc<Swapchain>,
    pub images: Vec<Arc<Image>>,
    pub gui: Gui,
}

impl RenderContext {
    pub fn init(event_loop: &ActiveEventLoop) -> Self {
        let instance = instance::new(event_loop);
        let window = Arc::new(
            event_loop
                .create_window(WindowAttributes::default().with_title("00-team-test-app"))
                .expect("create window"),
        );
        // let window = Arc::new(
        //     WindowBuilder::new()
        //         .with_title("00-team-test-app")
        //         .build(event_loop)
        //         .expect("window build"),
        // );
        let surface =
            Surface::from_window(instance.clone(), window.clone()).expect("surface from window");

        let viewport = Viewport {
            extent: window.inner_size().into(),
            ..Default::default()
        };

        let (device, queue) = device::get(&instance, &surface);
        let (swapchain, images) = swapchain::new(&device, &surface, &window);

        let allocators = Allocators::new(device.clone());

        let gui = Gui::new(
            event_loop,
            surface.clone(),
            queue.clone(),
            vulkano::format::Format::B8G8R8A8_UNORM,
            GuiConfig {
                is_overlay: true,
                ..Default::default()
            },
        );

        Self {
            instance,
            window,
            viewport,
            device,
            queue,
            allocators,
            swapchain,
            images,
            gui,
        }
    }
}
