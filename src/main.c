
#include <vulkan/vulkan.h>

#include "logger.h"
#include "stdio.h"

#define LOG_NAME "main"
#define unwrap(exp)                                                            \
    if ((result = exp)) {                                                      \
        log_trace(#exp " failed");                                             \
        cleanup();                                                             \
        return 1;                                                              \
    }

VkInstance            instance              = NULL;
VkSurfaceKHR          surface               = NULL;
VkResult              result                = 0;
VkDevice              device                = NULL;
VkPhysicalDevice      physical_device       = NULL;
VkDeviceMemory        in_memory             = NULL;
VkDeviceMemory        out_memory            = NULL;
VkBuffer              in_buffer             = NULL;
VkBuffer              out_buffer            = NULL;
VkShaderModule        shader_module         = NULL;
VkDescriptorSetLayout descriptor_set_layout = NULL;
VkPipelineLayout      pipeline_layout       = NULL;
VkPipelineCache       pipeline_cache        = NULL;
VkDescriptorPool      descriptor_pool       = NULL;
VkDescriptorSet       descriptor_set        = NULL;
VkCommandPool         cmd_pool              = NULL;
VkCommandBuffer       cmd_buffer            = NULL;
VkPipeline            compute_pipeline      = NULL;
VkFence               fence                 = NULL;

const char *vk_result_string(VkResult result);
void        cleanup(void);

int main(void) {
    log_info("init");

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

    uint32_t dev_count = 0;
    unwrap(vkEnumeratePhysicalDevices(instance, &dev_count, &physical_device));
    log_info("%d devices", dev_count);

    if (!dev_count) {
        log_error("no physical device was found");
        cleanup();
        return 1;
    }

    return 0;
}

void cleanup(void) {
    if (!instance || !device) return;
    if (cmd_pool)
        vkResetCommandPool(
            device, cmd_pool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT
        );
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
