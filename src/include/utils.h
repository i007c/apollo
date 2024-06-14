#ifndef __APOLLO_UTILS_H__
#define __APOLLO_UTILS_H__

#include <vulkan/vulkan.h>

#include "common.h"

const char *vk_result_string(VkResult result);
const char *status_string(status_t status);

#endif  // __APOLLO_UTILS_H__
