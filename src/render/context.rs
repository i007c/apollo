use std::sync::Arc;

use egui_winit_vulkano::Gui;
use vulkano::{
    device::{Device, Queue},
    image::Image,
    instance::Instance,
    pipeline::graphics::viewport::Viewport,
    swapchain::Surface,
};
use winit::{
    event_loop::EventLoop,
    window::{Window, WindowBuilder},
};

use crate::objects::{allocators::Allocators, device, instance};

pub struct RenderContext {
    instance: Instance,
    pub window: Arc<Window>,
    pub viewport: Viewport,
    pub device: Arc<Device>,
    pub queue: Arc<Queue>,
    pub allocators: Allocators,
    pub images: Vec<Arc<Image>>,
    pub gui: Gui,
}

impl RenderContext {
    pub fn init(event_loop: &EventLoop<()>) {
        let instance = instance::new(event_loop);
        let window = Arc::new(
            WindowBuilder::new()
                .with_title("Apollo")
                .build(event_loop)
                .expect("window build"),
        );
        let surface =
            Surface::from_window(instance.clone(), window.clone()).expect("surface from window");

        let viewport = Viewport {
            extent: window.inner_size().into(),
            ..Default::default()
        };

        let (device, queue) = device::get(&instance, &surface);
    }
}
