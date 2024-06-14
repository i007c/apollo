
#define LOG_NAME "vulkan"

#include "vulkan.h"

#include <stdlib.h>

#include <string.h>

#include "logger.h"
#include "utils.h"

extern VkInstance            instance;
extern VkPhysicalDevice      physical_device;
extern VkSurfaceKHR          surface;
extern VkDevice              device;
extern VkPhysicalDevice      physical_device;
extern VkDeviceMemory        in_memory;
extern VkDeviceMemory        out_memory;
extern VkBuffer              in_buffer;
extern VkBuffer              out_buffer;
extern VkShaderModule        shader_module;
extern VkDescriptorSetLayout descriptor_set_layout;
extern VkPipelineLayout      pipeline_layout;
extern VkPipelineCache       pipeline_cache;
extern VkDescriptorPool      descriptor_pool;
extern VkDescriptorSet       descriptor_set;
extern VkCommandPool         cmd_pool;
extern VkCommandBuffer       cmd_buffer;
extern VkPipeline            compute_pipeline;
extern VkFence               fence;

status_r vulkan_init(Vec *ext_list) {
    status_t status = OK;
    VkResult result = 0;

    VkInstanceCreateInfo UNUSED(create_info) = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    };
    Vec props;
    vk_unwrap(vkEnumerateInstanceExtensionProperties(NULL, &props.total, NULL));
    props.size = sizeof(VkExtensionProperties);
    vec_new(&props);
    vk_unwrap(vkEnumerateInstanceExtensionProperties(
        NULL, &props.count, (VkExtensionProperties *)props.items
    ));
    for (uint32_t i = 0; i < props.count; ++i) {
        VkExtensionProperties *p = (VkExtensionProperties *)&props.items[i];
        if (!strcmp(p->extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            vec_push(ext_list, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }
    }

    return status;
}

int vulkan_init_2(void) {
    VkResult result = 0;

    VkApplicationInfo app_info = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "Apollo",
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName        = NULL,
        .engineVersion      = VK_MAKE_VERSION(0, 0, 0),
        .apiVersion         = VK_API_VERSION_1_3,
    };

    VkInstanceCreateInfo create_info = {
        .sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
    };

    vk_unwrap(vkCreateInstance(&create_info, NULL, &instance));

    uint32_t dev_count = 100;
    vk_unwrap(vkEnumeratePhysicalDevices(instance, &dev_count, &physical_device));
    log_info("%d devices", dev_count);

    if (!dev_count) {
        log_error("no physical device (GPU) was found");
        vulkan_cleanup();
        return 1;
    }

    // uint32_t compute_queue_family = 0;
    uint32_t queue_family_count = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);

    VkQueueFamilyProperties *queue_family_list =
        malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_list);

    uint32_t queue_index = 0;
    for (uint32_t i = 0; i < queue_family_count; i++) {
        VkQueueFlags flags = queue_family_list[i].queueFlags;
        if (flags & VK_QUEUE_GRAPHICS_BIT) {
            queue_index = i;
            break;
        }
    }
    log_info("queue index: %d", queue_index);

    return 0;
}

void vulkan_cleanup(void) {
    if (!instance || !device) return;
    if (cmd_pool)
        vkResetCommandPool(device, cmd_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    if (fence) vkDestroyFence(device, fence, NULL);
    if (descriptor_set_layout)
        vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);

    if (pipeline_layout) vkDestroyPipelineLayout(device, pipeline_layout, NULL);
    if (pipeline_cache) vkDestroyPipelineCache(device, pipeline_cache, NULL);

    if (cmd_pool && cmd_buffer)
        vkFreeCommandBuffers(device, cmd_pool, 1, &cmd_buffer);

    if (shader_module) vkDestroyShaderModule(device, shader_module, NULL);
    if (compute_pipeline) vkDestroyPipeline(device, compute_pipeline, NULL);
    if (descriptor_pool) vkDestroyDescriptorPool(device, descriptor_pool, NULL);
    if (cmd_pool) vkDestroyCommandPool(device, cmd_pool, NULL);

    if (in_buffer) vkDestroyBuffer(device, in_buffer, NULL);
    if (out_buffer) vkDestroyBuffer(device, out_buffer, NULL);

    if (in_memory) vkFreeMemory(device, in_memory, NULL);
    if (out_memory) vkFreeMemory(device, out_memory, NULL);

    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);
}
