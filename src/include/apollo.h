
#ifndef __APOLLO_APOLLO_H__
#define __APOLLO_APOLLO_H__

#include <stdint.h>
#include <stdbool.h>

#if defined(__GNUC__) || defined(__clang__)
    #define UNUSED(name) _unused_ ## name __attribute__((unused))
#else
    #define UNUSED(name) _unused_ ## name
#endif

#define LEN(x) sizeof(x) / sizeof(x[0])

bool shader_load(char *path, uint32_t type);
bool shader_link(void);
bool shader_reload(int watch_fd);

#endif //__APOLLO_APOLLO_H__
