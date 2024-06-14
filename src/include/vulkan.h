#ifndef __APOLLO_VULKAN_H__
#define __APOLLO_VULKAN_H__

#include "common.h"
#include "vec.h"

status_r vulkan_init(Vec *vec);
void vulkan_cleanup(void);

#endif  // __APOLLO_VULKAN_H__
