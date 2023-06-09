
#ifndef __APOLLO_COMMON_H__
#define __APOLLO_COMMON_H__

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

#include <time.h>
#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <endian.h>

#include <sys/random.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

// #include <glad/glad.h>
#include <GLFW/glfw3.h>

#if defined(__GNUC__) || defined(__clang__)
    #define UNUSED(name) _unused_ ## name __attribute__((unused))
#else
    #define UNUSED(name) _unused_ ## name
#endif

typedef enum status_t {
    STS_SUCCESS = 0,
    // STS_BAD_ID = 4001,
    // STS_BAD_INDEX = 4002,
    // STS_FORBIDDEN = 4003,
    // STS_DB_ERROR = 5001,
    // STS_BAD_REQUEST_ARGS = 7001,
} status_t;

#endif //__APOLLO_COMMON_H__

