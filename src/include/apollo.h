
#ifndef __APOLLO_APOLLO_H__
#define __APOLLO_APOLLO_H__

#include "common.h"
#include "logger.h"

void shader_load(uint32_t program, const char *path, GLenum type);
void shader_link(uint32_t program);

#endif //__APOLLO_APOLLO_H__
