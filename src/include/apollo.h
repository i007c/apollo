
#ifndef __APOLLO_APOLLO_H__
#define __APOLLO_APOLLO_H__

#include <stdint.h>
#include <stdbool.h>

#include "cglm/types.h"

#if defined(__GNUC__) || defined(__clang__)
    #define UNUSED(name) _unused_ ## name __attribute__((unused))
#else
    #define UNUSED(name) _unused_ ## name
#endif

#define LEN(x) sizeof(x) / sizeof(x[0])


typedef enum status_t {
    STS_SUCCESS = 0,
    STS_WIN_ERR = 1,
} status_t;

typedef struct State {
    uint32_t width;
    uint32_t height;

    vec3 eye;
    vec3 center;
    vec3 up;

    float move_speed;

    vec4 curr_quat;
    vec4 prev_quat;
    
    bool mouse_left_pressed;
    bool mouse_middle_pressed;
    bool mouse_right_pressed;
    
    float mouse_x_last;
    float mouse_y_last;
    
    float zoom;

    bool wireframe;
} State;

void shader_load(uint32_t program, const char *path, uint32_t type);
void shader_link(uint32_t program);


#define TRACKBALLSIZE (0.6)
void trackball(float q[4], float p1x, float p1y, float p2x, float p2y);
void add_quats(vec4 q1, vec4 q2, vec4 dest);
void negate_quat(float *q, float *qn);
void build_rotmatrix(float m[4][4], const float q[4]);
void axis_to_quat(float a[3], float phi, float q[4]);
float tb_project_to_sphere(float, float, float);
void normalize_quat(float[4]);

#endif //__APOLLO_APOLLO_H__
