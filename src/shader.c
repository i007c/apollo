

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "gl.h"
#include "apollo.h"
#include "logger.h"

#define LS SECTOR_MAIN_SHADER
#define INFO_SIZE 512

int success = false;
char info[INFO_SIZE];

void shader_load(uint32_t program, const char *path, GLenum type) {
    log_verbose("loading '%s'", path);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        log_error(
            "shader '%s' can't be opened. %d. %s",
            path, errno, strerror(errno)
        );
        return;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size < 0) {
        log_error(
            "shader '%s' can't be seeked. %d. %s",
            path, errno, strerror(errno)
        );
        return;
    }

    if (!file_size) {
        log_error("shader '%s' is empty", path);
        return;
    }

    char *buffer = malloc(file_size);
    if (buffer == NULL) {
        log_error(
            "faild allocating buffer[%d] - %d. %s",
            file_size, errno, strerror(errno)
        );
        return;
    }

    assert(lseek(fd, 0, SEEK_SET) == 0);
    ssize_t read_size = read(fd, buffer, file_size);
    if (read_size < 0) {
        log_error(
            "faild read shader '%s' - %d. %s",
            path, errno, strerror(errno)
        );
        return;
    }

    if (read_size != file_size) {
        log_error(
            "invalid read size: %ld != %d - %d. %s",
            read_size, file_size, errno, strerror(errno)
        );
        return;
    }

    uint32_t shader = glCreateShader(type);
    glShaderSource(shader, 1, (const char **)&buffer, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, INFO_SIZE, NULL, info);
        log_error("shader '%s' compilation failed - %s", path, info);
        return;
    }

    glAttachShader(program, shader);

    glDeleteShader(shader);
    free(buffer);
}


void shader_link(uint32_t program) {
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, INFO_SIZE, NULL, info);
        log_error("shader program failed - %s", info);
    }
}
