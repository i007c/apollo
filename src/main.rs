use std::sync::Arc;

use vulkano::{
    buffer::{Buffer, BufferContents, BufferCreateInfo, BufferUsage},
    device::{
        physical::PhysicalDeviceType, Device, DeviceCreateInfo, DeviceExtensions, QueueCreateInfo,
        QueueFlags,
    },
    image::ImageUsage,
    instance::{Instance, InstanceCreateFlags, InstanceCreateInfo},
    memory::allocator::{AllocationCreateInfo, MemoryTypeFilter, StandardMemoryAllocator},
    pipeline::graphics::vertex_input::{Vertex, VertexDefinition},
    swapchain::{Surface, Swapchain, SwapchainCreateInfo},
    VulkanLibrary,
};
use winit::{
    event_loop::EventLoop,
    window::{Theme, WindowBuilder},
};

fn main() {
    dotenvy::from_path(".env").expect("could not load .env");
    pretty_env_logger::init();

    let event_loop = EventLoop::new();
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

    let window = Arc::new(
        WindowBuilder::new()
            .with_title("Apollo")
            .with_theme(Some(Theme::Dark))
            .build(&event_loop)
            .expect("window builder"),
    );
    let surface = Surface::from_window(instance.clone(), window.clone()).expect("surface");

    let device_extensions = DeviceExtensions {
        khr_swapchain: true,
        ..Default::default()
    };

    let (physical_device, queue_family_index) = instance
        .enumerate_physical_devices()
        .expect("enumerate physical devices")
        .filter(|d| d.supported_extensions().contains(&device_extensions))
        .filter_map(|d| {
            d.queue_family_properties()
                .iter()
                .enumerate()
                .position(|(i, q)| {
                    q.queue_flags.intersects(QueueFlags::GRAPHICS)
                        && d.surface_support(i as u32, &surface).unwrap_or(false)
                })
                .map(|i| (d, i as u32))
        })
        .min_by_key(|(p, _)| match p.properties().device_type {
            PhysicalDeviceType::DiscreteGpu => 0,
            PhysicalDeviceType::IntegratedGpu => 1,
            PhysicalDeviceType::VirtualGpu => 2,
            PhysicalDeviceType::Cpu => 3,
            PhysicalDeviceType::Other => 4,
            _ => 5,
        })
        .expect("no device found");

    log::debug!(
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
    .expect("device creation");

    let queue = queues.next().expect("no queue was created");

    let (mut swapchain, images) = {
        let surface_capabilities = device
            .physical_device()
            .surface_capabilities(&surface, Default::default())
            .expect("surface capabilities");

        let image_format = device
            .physical_device()
            .surface_formats(&surface, Default::default())
            .expect("surface foramts")[0]
            .0;

        Swapchain::new(
            device.clone(),
            surface,
            SwapchainCreateInfo {
                min_image_count: surface_capabilities.min_image_count.max(2),
                image_format,
                image_extent: window.inner_size().into(),
                image_usage: ImageUsage::COLOR_ATTACHMENT,
                composite_alpha: surface_capabilities
                    .supported_composite_alpha
                    .into_iter()
                    .next()
                    .expect("no supported composite alpha"),
                ..Default::default()
            },
        )
        .expect("swapchain creation")
    };

    let memory_allocator = Arc::new(StandardMemoryAllocator::new_default(device.clone()));

    #[derive(BufferContents, Vertex)]
    #[repr(C)]
    struct Vertex {
        #[format(R32G32_SFLOAT)]
        position: [f32; 2],
    }

    let verteces = [
        Vertex {
            position: [-0.5, -0.25],
        },
        Vertex {
            position: [0.0, 0.5],
        },
        Vertex {
            position: [0.25, -0.1],
        },
    ];
    let vertex_buffer = Buffer::from_iter(
        memory_allocator,
        BufferCreateInfo {
            usage: BufferUsage::VERTEX_BUFFER,
            ..Default::default()
        },
        AllocationCreateInfo {
            memory_type_filter: MemoryTypeFilter::PREFER_DEVICE
                | MemoryTypeFilter::HOST_SEQUENTIAL_WRITE,
            ..Default::default()
        },
        verteces,
    )
    .expect("buffer from iter");

    mod vs {
        vulkano_shaders::shader! {
            ty: "vertex",
            src: r"
                #version 450

                layout(location = 0) in vec2 position;
                
                void main() {
                    gl_Position = vec4(position, 0.0, 1.0);
                }
            ",
        }
    }

    mod fs {
        vulkano_shaders::shader! {
            ty: "fragment",
            src: r"
                #version 450

                layout(location = 0) out vec4 f_color;

                void main() {
                    f_color = vec4(1.0, 0.0, 0.0, 1.0);
                }
            ",
        }
    }

    let render_pass = vulkano::single_pass_renderpass!(
        device.clone(),
        attachments: {
            color: {
                format: swapchain.image_format(),
                samples: 1,
                load_op: Clear,
                store_op: Store
            }
        },
        pass: {
            color: [color],
            depth_stencil: {}
        }
    )
    .expect("render pass");

    let pipeline = {
        let vs = vs::load(device.clone())
            .expect("vertex shader load")
            .entry_point("main")
            .expect("vertex shader entry point");

        let fs = fs::load(device.clone())
            .expect("fragment shader load")
            .entry_point("main")
            .expect("fragment shader entry point");

        let vertex_input_state = Vertex::per_vertex()
            .definition(&vs.info().input_interface)
            .expect("vertex definitions");
    };
}
