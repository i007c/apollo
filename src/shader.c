

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/inotify.h>

#include "gl.h"
#include "apollo.h"
#include "logger.h"

#define LS SECTOR_MAIN_SHADER
#define INFO_SIZE 512


extern State state;

int shader_status = false;
char info[INFO_SIZE];

typedef struct ShaderState {
    char *path;
    uint32_t type;
    int watch_fd;
    uint32_t shader_id;

    struct ShaderState *next;
    struct ShaderState *prev;
} ShaderState;

ShaderState *shader_state = NULL;

status_t shader_read(char *path, char **result) {
    log_info("reading shader '%s'", path);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        log_error(
            "shader '%s' can't be opened. %d. %s",
            path, errno, strerror(errno)
        );
        return STS_GLS_ERR;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size < 0) {
        log_error(
            "shader '%s' can't be seeked. %d. %s",
            path, errno, strerror(errno)
        );
        return STS_GLS_ERR;
    }

    if (!file_size) {
        log_error("shader '%s' is empty", path);
        return STS_GLS_ERR;
    }

    *result = malloc(file_size);
    if (*result == NULL) {
        log_error(
            "faild allocating result[%d] - %d. %s",
            file_size, errno, strerror(errno)
        );
        return STS_GLS_ERR;
    }

    assert(lseek(fd, 0, SEEK_SET) == 0);
    ssize_t read_size = read(fd, *result, file_size);
    if (read_size < 0) {
        log_error(
            "faild read shader '%s' - %d. %s",
            path, errno, strerror(errno)
        );
        return STS_GLS_ERR;
    }

    if (read_size != file_size) {
        log_error(
            "invalid read size: %ld != %d - %d. %s",
            read_size, file_size, errno, strerror(errno)
        );
        return STS_GLS_ERR;
    }

    return STS_SUCCESS;
}

status_t shader_load(char *path, GLenum type) {
    status_t status;
    ShaderState *curent_state = NULL;
    log_verbose("loading '%s'", path);

    curent_state = malloc(sizeof(ShaderState));
    if (curent_state == NULL) {
        log_errno("malloc faild to allocate a shader state", errno);
        return STS_GLS_ERR;
    }

    curent_state->next = NULL;
    curent_state->path = path;
    curent_state->type = type;

    if (shader_state == NULL) {
        shader_state = curent_state;
        shader_state->prev = curent_state;
    } else {
        // shader_state prev is also the last state
        // end element in the chain
        shader_state->prev->next = curent_state;
        curent_state->prev = shader_state->prev;
        shader_state->prev = curent_state;
    }

    log_info("add watcher for '%s'", path);
    curent_state->watch_fd = inotify_add_watch(
        state.inotify_fd, path,
        IN_MODIFY | IN_MOVE_SELF
    );
    if (curent_state->watch_fd < 0) {
        log_errno("can't watch shader: %s.", path, errno);
        return STS_GLS_ERR;
    }

    char *buffer = NULL;
    if ((status = shader_read(path, &buffer)))
        return status;


    log_info("creating and compiling the shader: '%s'", path);
    curent_state->shader_id = glCreateShader(type);
    glShaderSource(curent_state->shader_id, 1, (const char **)&buffer, NULL);
    glCompileShader(curent_state->shader_id);

    glGetShaderiv(curent_state->shader_id, GL_COMPILE_STATUS, &shader_status);
    if (!shader_status) {
        glGetShaderInfoLog(curent_state->shader_id, INFO_SIZE, NULL, info);
        log_error("shader '%s' compilation failed - %s", path, info);
        free(buffer);
        return STS_GLS_ERR;
    }

    glAttachShader(state.gl_program, curent_state->shader_id);

    glDeleteShader(curent_state->shader_id);
    free(buffer);

    return STS_SUCCESS;
}

status_t shader_reload(int watch_fd) {
    log_info("reloading shader: %d", watch_fd);

    status_t status;
    char *buffer = NULL;
    ShaderState *curent_state = shader_state;

    while (curent_state != NULL) {
        if (curent_state->watch_fd == watch_fd) {
            if ((status = shader_read(curent_state->path, &buffer)))
                return status;

            glDetachShader(state.gl_program, curent_state->shader_id);

            curent_state->shader_id = glCreateShader(curent_state->type);
            glShaderSource(
                curent_state->shader_id, 1, (const char **)&buffer, NULL
            );
            glCompileShader(curent_state->shader_id);
            glGetShaderiv(
                curent_state->shader_id, GL_COMPILE_STATUS, &shader_status
            );
            free(buffer);

            if (!shader_status) {
                glGetShaderInfoLog(
                    curent_state->shader_id, INFO_SIZE, NULL, info
                );
                log_error(
                    "shader '%s' compilation failed - %s",
                    curent_state->path, info
                );
                return STS_GLS_ERR;
            }

            glAttachShader(state.gl_program, curent_state->shader_id);
            glDeleteShader(curent_state->shader_id);

            return STS_SUCCESS;
        }

        curent_state = shader_state->next;
    }

    log_warn("shader with watch fd: %d was not found.", watch_fd);
    return STS_GLS_ERR;
}


status_t shader_link(void) {
    glLinkProgram(state.gl_program);

    glGetProgramiv(state.gl_program, GL_LINK_STATUS, &shader_status);
    if (!shader_status) {
        glGetProgramInfoLog(state.gl_program, INFO_SIZE, NULL, info);
        log_error("shader program failed - %s", info);
        return STS_GLS_ERR;
    }

    return STS_SUCCESS;
}
