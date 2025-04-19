use egui_winit_vulkano::{Gui, GuiConfig};
use std::{sync::Arc, time::Instant};
use vulkano::format::Format;
use vulkano::image::ImageUsage;
use vulkano::image::view::ImageView;
use vulkano::swapchain::{Surface, Swapchain, SwapchainCreateInfo};
use vulkano::{
    pipeline::GraphicsPipeline,
    render_pass::{Framebuffer, RenderPass},
    shader::EntryPoint,
    sync::GpuFuture,
};
use winit::event_loop::ActiveEventLoop;
use winit::{
    // event_loop::ActiveEventLoop,
    window::Window,
};

use crate::app::ApolloApp;

// use crate::object::{allocators::Allocators, device, instance, swapchain};

pub struct RenderContext {
    pub window: Arc<Window>,
    pub swapchain: Arc<Swapchain>,
    pub render_pass: Arc<RenderPass>,
    pub framebuffers: Vec<Arc<Framebuffer>>,
    pub vs: EntryPoint,
    pub fs: EntryPoint,
    pub pipeline: Arc<GraphicsPipeline>,
    pub recreate_swapchain: bool,
    pub previous_frame_end: Option<Box<dyn GpuFuture>>,
    pub rotation_start: Instant,
    // instance: Arc<Instance>,
    // pub window: Arc<Window>,
    // pub viewport: Viewport,
    // pub device: Arc<Device>,
    // pub queue: Arc<Queue>,
    // pub allocators: Allocators,
    // pub swapchain: Arc<Swapchain>,
    // pub images: Vec<Arc<ImageView>>,
    pub gui: Gui,
    // pub vertex_buffer: Subbuffer<[Position]>,
    // pub normals_buffer: Subbuffer<[Normal]>,
    // pub index_buffer: Subbuffer<[u16]>,
    // pub uniform_buffer: SubbufferAllocator,
    // pub render_pass: Arc<RenderPass>,
    // pub vs: EntryPoint,
    // pub fs: EntryPoint,
    // pub pipeline: Arc<GraphicsPipeline>,
    // pub framebuffers: Vec<Arc<Framebuffer>>,
}

impl RenderContext {
    pub fn new(event_loop: &ActiveEventLoop, app: &ApolloApp) -> Self {
        let window = Arc::new(
            event_loop
                .create_window(
                    Window::default_attributes().with_title("00-team-test-app"),
                )
                .unwrap(),
        );
        let surface =
            Surface::from_window(app.instance.clone(), window.clone()).unwrap();
        let window_size = window.inner_size();

        let (swapchain, images) = {
            let surface_capabilities = app
                .device
                .physical_device()
                .surface_capabilities(&surface, Default::default())
                .unwrap();
            let (image_format, _) = app
                .device
                .physical_device()
                .surface_formats(&surface, Default::default())
                .unwrap()[0];

            Swapchain::new(
                app.device.clone(),
                surface.clone(),
                SwapchainCreateInfo {
                    min_image_count: surface_capabilities
                        .min_image_count
                        .max(2),
                    image_format,
                    image_extent: window_size.into(),
                    image_usage: ImageUsage::COLOR_ATTACHMENT,
                    composite_alpha: surface_capabilities
                        .supported_composite_alpha
                        .into_iter()
                        .next()
                        .unwrap(),
                    ..Default::default()
                },
            )
            .unwrap()
        };

        let render_pass = vulkano::single_pass_renderpass!(
            app.device.clone(),
            attachments: {
                color: {
                    format: swapchain.image_format(),
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

        let vs = crate::shader::vs::load(app.device.clone())
            .unwrap()
            .entry_point("main")
            .unwrap();
        let fs = crate::shader::fs::load(app.device.clone())
            .unwrap()
            .entry_point("main")
            .unwrap();

        let (framebuffers, pipeline) =
            crate::utils::window_size_dependent_setup(
                window_size,
                &images,
                &render_pass,
                &app.memory_allocator,
                &vs,
                &fs,
            );

        let previous_frame_end =
            Some(vulkano::sync::now(app.device.clone()).boxed());

        let rotation_start = Instant::now();

        let gui = Gui::new(
            event_loop,
            surface.clone(),
            app.queue.clone(),
            vulkano::format::Format::B8G8R8A8_UNORM,
            GuiConfig { is_overlay: true, ..Default::default() },
        );

        // let images = images
        //     .into_iter()
        //     .map(|image| ImageView::new_default(image).unwrap())
        //     .collect::<Vec<_>>();

        Self {
            window,
            swapchain,
            render_pass,
            // images,
            framebuffers,
            vs,
            fs,
            gui,
            pipeline,
            recreate_swapchain: false,
            previous_frame_end,
            rotation_start,
        }
    }
}

// impl RenderContext {
//     pub fn init(event_loop: &ActiveEventLoop) -> Self {
//         let instance = instance::new(event_loop);
//         let window = Arc::new(
//             event_loop
//                 .create_window(WindowAttributes::default().with_title("00-team-test-app"))
//                 .expect("create window"),
//         );
//         // let window = Arc::new(
//         //     WindowBuilder::new()
//         //         .with_title("00-team-test-app")
//         //         .build(event_loop)
//         //         .expect("window build"),
//         // );
//         let surface =
//             Surface::from_window(instance.clone(), window.clone()).expect("surface from window");
//
//         let viewport = Viewport {
//             extent: window.inner_size().into(),
//             ..Default::default()
//         };
//
//         let (device, queue) = device::get(&instance, &surface);
//         let (swapchain, images) = swapchain::new(&device, &surface, &window);
//
//         let allocators = Allocators::new(device.clone());
//
//         let gui = Gui::new(
//             event_loop,
//             surface.clone(),
//             queue.clone(),
//             vulkano::format::Format::B8G8R8A8_UNORM,
//             GuiConfig {
//                 is_overlay: true,
//                 ..Default::default()
//             },
//         );
//
//         let vertex_buffer = Buffer::from_iter(
//             allocators.memory.clone(),
//             BufferCreateInfo {
//                 usage: BufferUsage::VERTEX_BUFFER,
//                 ..Default::default()
//             },
//             AllocationCreateInfo {
//                 memory_type_filter: MemoryTypeFilter::PREFER_DEVICE
//                     | MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
//                 ..Default::default()
//             },
//             POSITIONS,
//         )
//         .unwrap();
//
//         let normals_buffer = Buffer::from_iter(
//             allocators.memory.clone(),
//             BufferCreateInfo {
//                 usage: BufferUsage::VERTEX_BUFFER,
//                 ..Default::default()
//             },
//             AllocationCreateInfo {
//                 memory_type_filter: MemoryTypeFilter::PREFER_DEVICE
//                     | MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
//                 ..Default::default()
//             },
//             NORMALS,
//         )
//         .unwrap();
//
//         let index_buffer = Buffer::from_iter(
//             allocators.memory.clone(),
//             BufferCreateInfo {
//                 usage: BufferUsage::INDEX_BUFFER,
//                 ..Default::default()
//             },
//             AllocationCreateInfo {
//                 memory_type_filter: MemoryTypeFilter::PREFER_DEVICE
//                     | MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
//                 ..Default::default()
//             },
//             INDICES,
//         )
//         .unwrap();
//
//         let uniform_buffer = SubbufferAllocator::new(
//             allocators.memory.clone(),
//             SubbufferAllocatorCreateInfo {
//                 buffer_usage: BufferUsage::UNIFORM_BUFFER,
//                 memory_type_filter: MemoryTypeFilter::PREFER_DEVICE
//                     | MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
//                 ..Default::default()
//             },
//         );
//
//         log::info!("swapchain.image_format(): {:?}", swapchain.image_format());
//         let render_pass = vulkano::single_pass_renderpass!(
//             device.clone(),
//             attachments: {
//                 color: {
//                     format: swapchain.image_format(),
//                     samples: 1,
//                     load_op: Clear,
//                     store_op: Store,
//                 },
//                 depth_stencil: {
//                     format: Format::D16_UNORM,
//                     samples: 1,
//                     load_op: Clear,
//                     store_op: DontCare,
//                 },
//             },
//             pass: {
//                 color: [color],
//                 depth_stencil: {depth_stencil},
//             },
//         )
//         .unwrap();
//
//         let vs = crate::shader::vs::load(device.clone())
//             .unwrap()
//             .entry_point("main")
//             .unwrap();
//         let fs = crate::shader::fs::load(device.clone())
//             .unwrap()
//             .entry_point("main")
//             .unwrap();
//
//         let (pipeline, framebuffers) = crate::utils::window_size_dependent_setup(
//             allocators.memory.clone(),
//             vs.clone(),
//             fs.clone(),
//             &images,
//             render_pass.clone(),
//         );
//
//         Self {
//             instance,
//             window,
//             viewport,
//             device,
//             queue,
//             allocators,
//             swapchain,
//             images,
//             gui,
//             index_buffer,
//             vertex_buffer,
//             vs,
//             fs,
//             pipeline,
//             render_pass,
//             framebuffers,
//             normals_buffer,
//             uniform_buffer,
//         }
//     }
// }
