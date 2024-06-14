use vulkano::{swapchain::Surface, VulkanLibrary};
use winit::event_loop::EventLoop;

fn main() {
    dotenvy::from_path(".env").expect("could not load .env");
    pretty_env_logger::init();

    let event_loop = EventLoop::new().expect("event loop");
    let library = VulkanLibrary::new().expect("vulkan library");

    let required_extensions = Surface::required_extensions(&event_loop);
    log::debug!("required_extensions: {required_extensions:#?}");
}
