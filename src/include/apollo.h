
#ifndef __APOLLO_APOLLO_H__
#define __APOLLO_APOLLO_H__

#include <stdbool.h>
#include <stdint.h>

#include <vulkan/vulkan.h>

bool shader_load(char *path, uint32_t type);
bool shader_link(void);
bool shader_reload(int watch_fd);

#endif  //__APOLLO_APOLLO_H__
