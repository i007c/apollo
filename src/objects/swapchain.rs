use std::sync::Arc;

use vulkano::{
    device::Device,
    image::{Image, ImageUsage},
    swapchain::{Surface, Swapchain, SwapchainCreateFlags, SwapchainCreateInfo},
};
use winit::window::Window;

pub fn new(
    device: &Arc<Device>,
    surface: &Arc<Surface>,
    window: &Arc<Window>,
) -> (Arc<Swapchain>, Vec<Arc<Image>>) {
    let surface_capabilities = device
        .physical_device()
        .surface_capabilities(&surface, Default::default())
        .expect("surface capabilities");

    // let image_format = device
    //     .physical_device()
    //     .surface_formats(&surface, Default::default())
    //     .expect("surface formats")[0]
    //     .0;

    let image_format = vulkano::format::Format::B8G8R8A8_SRGB;
    let gui_format = vulkano::format::Format::B8G8R8A8_UNORM;

    Swapchain::new(
        device.clone(),
        surface.clone(),
        SwapchainCreateInfo {
            flags: SwapchainCreateFlags::MUTABLE_FORMAT,
            min_image_count: surface_capabilities.min_image_count.max(2),
            image_view_formats: vec![image_format, gui_format],
            image_format,
            image_extent: window.inner_size().into(),
            image_usage: ImageUsage::COLOR_ATTACHMENT,
            composite_alpha: surface_capabilities
                .supported_composite_alpha
                .into_iter()
                .next()
                .expect("no composite alpha"),
            ..Default::default()
        },
    )
    .expect("swapchain creation faild")
}
