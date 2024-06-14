
#include <stdlib.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <vulkan/vulkan.h>

#include "common.h"
#include "logger.h"
#include "utils.h"
#include "window.h"

#define LOG_NAME "main"

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


int main(void) {
    status_t status = OK;
    log_info("init");

    unwrap_log(window_init());

    return 0;
}


