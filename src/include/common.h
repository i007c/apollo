
#ifndef __APOLLO_COMMON_H__
#define __APOLLO_COMMON_H__

#include <stdint.h>

#define LEN(array) sizeof(array) / sizeof(array[0])

#if defined(__GNUC__) || defined(__clang__)
#define UNUSED(name) _unused_##name __attribute__((unused))
#else
#define UNUSED(name) _unused_##name
#endif

#define unwrap(exp)                                                            \
    if ((status = exp)) {                                                      \
        return status;                                                         \
    }

#define vk_unwrap(exp)                                                         \
    if ((result = exp)) {                                                      \
        log_trace(#exp ": %s", vk_result_string(result));                      \
        cleanup();                                                             \
        return 1;                                                              \
    }

#define unwrap_log(exp)                                                        \
    if ((status = exp)) {                                                      \
        log_trace(#exp ": [%d]: %s", status, status_string(status));           \
        return status;                                                         \
    }

typedef enum status_t {
    OK = 0,
    ERR_VK_ERROR,
    ERR_SDL_WIN_CRATE,
    ERR_SDL_VK_EXT,
} status_t;

#define status_r status_t __attribute__((warn_unused_result))

#endif  // __APOLLO_COMMON_H__
