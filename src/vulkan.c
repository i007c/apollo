
#define LOG_NAME "vulkan"

#include <stdlib.h>

#include "logger.h"
#include "utils.h"

extern VkInstance       instance;
extern VkPhysicalDevice physical_device;

void cleanup(void);

int vulkan_init(void) {
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

    unwrap(vkCreateInstance(&create_info, NULL, &instance));

    uint32_t dev_count = 100;
    unwrap(vkEnumeratePhysicalDevices(instance, &dev_count, &physical_device));
    log_info("%d devices", dev_count);

    if (!dev_count) {
        log_error("no physical device (GPU) was found");
        cleanup();
        return 1;
    }

    // uint32_t compute_queue_family = 0;
    uint32_t queue_family_count = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_count, NULL
    );

    VkQueueFamilyProperties *queue_family_list =
        malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_count, queue_family_list
    );

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
