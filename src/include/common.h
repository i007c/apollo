
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

#include <GL/glut.h>

#include "logger.h"

typedef enum status_t {
    STS_SUCCESS = 0,
    // STS_BAD_ID = 4001,
    // STS_BAD_INDEX = 4002,
    // STS_FORBIDDEN = 4003,
    // STS_DB_ERROR = 5001,
    // STS_BAD_REQUEST_ARGS = 7001,
} status_t;

#endif //__APOLLO_COMMON_H__

