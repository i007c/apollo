

#define GLAD_GL_IMPLEMENTATION
#include "gl.h"

#include "apollo.h"
#include "logger.h"

#include "cglm/cglm.h"

#include <time.h>

#include <GL/gl.h>
#include <GLFW/glfw3.h>

#define LS SECTOR_MAIN_APOLLO
#define PF "\033[32m%f\033[0m"

#define DONKEYOBJ_IMPLEMENTATION
#include "donk.h"



status_t window_init(GLFWwindow **win);

// vec3 cam_position;
// vec3 cam_front;
// vec3 cam_up;
// vec3 cam_right;
// vec3 cam_world_up;
// // euler Angles
// float cam_yaw = -90;
// float cam_pitch = -10;
// // camera options
// float cam_mouse_sensitivity = 0.0001;
// float cam_key_sensitivity = 1.2;
// float cam_fov = 45;
// vec3 cam_temp;


// void cam_logger(void) {
//     log_verbose(
//         "\ncam {\n  temp: [ "PF", "PF", "PF" ]\n  yaw: "PF"\n  pitch: "PF"\n"
//         "  position: [ "PF", "PF", "PF" ]\n  front: [ "PF", "PF", "PF" ]\n"
//         "  right: [ "PF", "PF", "PF" ]\n  up: [ "PF", "PF", "PF" ]\n"
//         "  world_up: [ "PF", "PF", "PF" ]\n  zoom: "PF"\n}\n",
//         cam_temp[0], cam_temp[1], cam_temp[2],
//         cam_yaw, cam_pitch,
//         cam_position[0], cam_position[1], cam_position[2],
//         cam_front[0], cam_front[1], cam_front[2],
//         cam_right[0], cam_right[1], cam_right[2],
//         cam_up[0], cam_up[1], cam_up[2],
//         cam_world_up[0], cam_world_up[1], cam_world_up[2],
//         cam_fov
//     );
// }
//


void debug_message(
    GLenum source, GLenum type, uint32_t id, GLenum severity,
    GLsizei length, const char *message, const void *UNUSED(P)
) {
    log_verbose(
        "\nsource: %d\ntype: %d\nid: %d\nseverity: %d\nlength: %d\n"
        "message: %s\n",
        source, type, id, severity, length, message
    );
}


State state;
void state_init(void) {
    log_info("state init");

    memset(&state, 0, sizeof(State));

    state.width = 1080;
    state.height = 720;
    state.move_speed = 256;
    state.zoom = 85.0f;

    state.wireframe = true;

    trackball(state.curr_quat, 0, 0, 0, 0);

    state.eye[2] = 10.0f;
    state.up[1] = 1.0f;
}

int main(void) {
    log_info("Starting Apollo");

    srand(time(NULL));

    state_init();

    GLFWwindow *window = NULL;
    if (window_init(&window)) {
        return EXIT_FAILURE;
    }
    assert(window != NULL);

    int version = gladLoadGL(glfwGetProcAddress);
    log_info("version: %d\n", version);

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debug_message, NULL);

    glEnable(GL_DEPTH_TEST);

    donk_t donk_result;
    donk_status_t status = donk("object/cow.obj", &donk_result);
    assert(status == DONK_SUCCESS);

    // for (size_t i = 0; i < ctx.vgi; i++) {
    //     if (i%3==0) printf("\n");
    //     printf(" v[%ld]: %f ", i, ctx.vertex_vg[i]);
    // }
    // printf("\n");
    //
    // for (size_t i = 0; i < ctx.ii; i++) {
    //     if (i%3==0) printf("\n");
    //     printf(" i[%ld]: %d ", i, ctx.indexes[i]);
    // }
    // printf("\n");

    // GLuint vertexbuffer;
    // glGenBuffers(1, &vertexbuffer);
    // glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    // GLuint colorbuffer;
    // glGenBuffers(1, &colorbuffer);
    // glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

    log_verbose("elements_count:  %ld", donk_result.elements_count);
    log_verbose("vertices_count:  %ld", donk_result.vertices_count);
    log_verbose("textures_count:  %ld", donk_result.textures_count);
    log_verbose("normals_count:  %ld", donk_result.normals_count);

    GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

    uint32_t vbuf = 0;
    glGenBuffers(1, &vbuf);
    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STATIC_DRAW);
    glBufferData(
        GL_ARRAY_BUFFER,
        donk_result.vertices_count * sizeof(float),
        donk_result.vertices,
        GL_STATIC_DRAW
    );
    // glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, 0);
    // glVertexAttribLPointer(0, 3, GL_DOUBLE, 0, 0);

    uint32_t ibuf = 0;
    glGenBuffers(1, &ibuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        donk_result.elements_count * sizeof(uint32_t),
        donk_result.elements,
        GL_STATIC_DRAW
    );
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ind), ind, GL_STATIC_DRAW);


    uint32_t shader_program = glCreateProgram();
    // shader_load(shader_program, "shader/vertex.glsl", GL_VERTEX_SHADER);
    shader_load(shader_program, "shader/cam/vx.glsl", GL_VERTEX_SHADER);
    shader_load(shader_program, "shader/fragment.glsl", GL_FRAGMENT_SHADER);
    shader_link(shader_program);
    glUseProgram(shader_program);

    int ucloc = glGetUniformLocation(shader_program, "u_color");

    int uloc_view = glGetUniformLocation(shader_program, "view");
    int uloc_projection = glGetUniformLocation(shader_program, "projection");
    int uloc_model = glGetUniformLocation(shader_program, "model");

    glClearColor(0.016f, 0.016f, 0.016f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // mat4 projection;
        // glm_mat4_identity(projection);
        //
        // // glm_ortho(0, width, height, 0, -1, 1, projection);
        // glm_perspective(cam_fov/PI180, width/height, 0.1, 100, projection);
        // glUniformMatrix4fv(uloc_projection, 1, GL_FALSE, &projection[0][0]);
        //
        // vec3 cam_pf;
        // glm_vec3_add(cam_position, cam_front, cam_pf);
        //
        // mat4 view;
        // glm_lookat(cam_position, cam_pf, cam_up, view);
        // glUniformMatrix4fv(uloc_view, 1, GL_FALSE, &view[0][0]);
        //
        // mat4 model;
        // // glm_translate(model, )
        // glm_mat4_identity(model);
        // glUniformMatrix4fv(uloc_model, 1, GL_FALSE, &model[0][0]);

        mat4 projection;
        // mat4s projection = glms_perspective(
        //     75.0f * (GLM_PI / 180.0f),
        // );
        // glm_ortho(
        //     0, (float)state.width / state.zoom,
        //     0.0f, (float)state.height / state.zoom,
        //     0.1f, 100.0f, projection
        // );
        // glm_ortho(0, (float)width, (float)height, 0, -1.0f, 1.0f, projection);
        glm_perspective(
            state.zoom * (GLM_PI / 180.0f),
            state.width / state.height,
            0.01f, 1000.0f, projection
        );
        glUniformMatrix4fv(uloc_projection, 1, GL_FALSE, &projection[0][0]);

        mat4 view;
        glm_mat4_identity(view);
        // vec3 x = {2, +2, -3};
        // log_verbose("center: "PF", "PF", "PF, center[0], center[1], center[2]);
        // glm_translate(view, center);
        glm_lookat(state.eye, state.center, state.up, view);
        glUniformMatrix4fv(uloc_view, 1, GL_FALSE, &view[0][0]);

        mat4 model;
        build_rotmatrix(model, state.curr_quat);
        glUniformMatrix4fv(uloc_model, 1, GL_FALSE, &model[0][0]);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        // glUniform4f(ucloc, (float)rand() / (float)RAND_MAX, 0.6, 1, 1);
        glUniform4f(ucloc, 0.32, 0.6, 1, 1);

        // glDrawArrays(GL_TRIANGLES, 0, LEN(cube) / 3);
        glDrawElements(
            GL_TRIANGLES,
            donk_result.elements_count * sizeof(uint32_t),
            GL_UNSIGNED_INT,
            0
        );
        // glDrawElements(GL_TRIANGLES, sizeof(ind), GL_UNSIGNED_INT, 0);

        if (state.wireframe) {
            glLineWidth(1);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glUniform4f(ucloc, 0, 0, 0, 1);
            glDrawElements(
                GL_TRIANGLES,
                donk_result.elements_count * sizeof(uint32_t),
                GL_UNSIGNED_INT,
                0
            );
        }


		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteBuffers(1, &vbuf);
    glDeleteBuffers(1, &ibuf);
    glDeleteProgram(shader_program);

    glfwTerminate();

    return EXIT_SUCCESS;
}









////////////////////////

static void vzero(float *v) {
  v[0] = 0.0;
  v[1] = 0.0;
  v[2] = 0.0;
}

static void vset(float *v, float x, float y, float z) {
  v[0] = x;
  v[1] = y;
  v[2] = z;
}

static void vsub(const float *src1, const float *src2, float *dst) {
  dst[0] = src1[0] - src2[0];
  dst[1] = src1[1] - src2[1];
  dst[2] = src1[2] - src2[2];
}

static void vcopy(const float *v1, float *v2) {
  register int i;
  for (i = 0; i < 3; i++)
    v2[i] = v1[i];
}

static void vcross(const float *v1, const float *v2, float *cross) {
  float temp[3];

  temp[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
  temp[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
  temp[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
  vcopy(temp, cross);
}

static float vlength(const float *v) {
  return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static void vscale(float *v, float div) {
  v[0] *= div;
  v[1] *= div;
  v[2] *= div;
}

static void vnormal(float *v) { vscale(v, 1.0 / vlength(v)); }

static float vdot(const float *v1, const float *v2) {
  return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

static void vadd(const float *src1, const float *src2, float *dst) {
  dst[0] = src1[0] + src2[0];
  dst[1] = src1[1] + src2[1];
  dst[2] = src1[2] + src2[2];
}

/*
 * Ok, simulate a track-ball.  Project the points onto the virtual
 * trackball, then figure out the axis of rotation, which is the cross
 * product of P1 P2 and O P1 (O is the center of the ball, 0,0,0)
 * Note:  This is a deformed trackball-- is a trackball in the center,
 * but is deformed into a hyperbolic sheet of rotation away from the
 * center.  This particular function was chosen after trying out
 * several variations.
 *
 * It is assumed that the arguments to this routine are in the range
 * (-1.0 ... 1.0)
 */
void trackball(float q[4], float p1x, float p1y, float p2x, float p2y) {
  float a[3]; /* Axis of rotation */
  float phi;  /* how much to rotate about axis */
  float p1[3], p2[3], d[3];
  float t;

  if (p1x == p2x && p1y == p2y) {
    /* Zero rotation */
    vzero(q);
    q[3] = 1.0;
    return;
  }

  /*
   * First, figure out z-coordinates for projection of P1 and P2 to
   * deformed sphere
   */
  vset(p1, p1x, p1y, tb_project_to_sphere(TRACKBALLSIZE, p1x, p1y));
  vset(p2, p2x, p2y, tb_project_to_sphere(TRACKBALLSIZE, p2x, p2y));

  /*
   *  Now, we want the cross product of P1 and P2
   */
  vcross(p2, p1, a);

  /*
   *  Figure out how much to rotate around that axis.
   */
  vsub(p1, p2, d);
  t = vlength(d) / (2.0 * TRACKBALLSIZE);

  /*
   * Avoid problems with out-of-control values...
   */
  if (t > 1.0)
    t = 1.0;
  if (t < -1.0)
    t = -1.0;
  phi = 2.0 * asin(t);

  axis_to_quat(a, phi, q);
}

/*
 *  Given an axis and angle, compute quaternion.
 */
void axis_to_quat(float a[3], float phi, float q[4]) {
  vnormal(a);
  vcopy(a, q);
  vscale(q, sin(phi / 2.0));
  q[3] = cos(phi / 2.0);
}

/*
 * Project an x,y pair onto a sphere of radius r OR a hyperbolic sheet
 * if we are away from the center of the sphere.
 */
float tb_project_to_sphere(float r, float x, float y) {
  float d, t, z;

  d = sqrt(x * x + y * y);
  if (d < r * 0.70710678118654752440) { /* Inside sphere */
    z = sqrt(r * r - d * d);
  } else { /* On hyperbola */
    t = r / 1.41421356237309504880;
    z = t * t / d;
  }
  return z;
}

/*
 * Given two rotations, e1 and e2, expressed as quaternion rotations,
 * figure out the equivalent single rotation and stuff it into dest.
 *
 * This routine also normalizes the result every RENORMCOUNT times it is
 * called, to keep error from creeping in.
 *
 * NOTE: This routine is written so that q1 or q2 may be the same
 * as dest (or each other).
 */

#define RENORMCOUNT 97

void add_quats(float q1[4], float q2[4], float dest[4]) {
  static int count = 0;
  float t1[4], t2[4], t3[4];
  float tf[4];

  vcopy(q1, t1);
  vscale(t1, q2[3]);

  vcopy(q2, t2);
  vscale(t2, q1[3]);

  vcross(q2, q1, t3);
  vadd(t1, t2, tf);
  vadd(t3, tf, tf);
  tf[3] = q1[3] * q2[3] - vdot(q1, q2);

  dest[0] = tf[0];
  dest[1] = tf[1];
  dest[2] = tf[2];
  dest[3] = tf[3];

  if (++count > RENORMCOUNT) {
    count = 0;
    normalize_quat(dest);
  }
}

/*
 * Quaternions always obey:  a^2 + b^2 + c^2 + d^2 = 1.0
 * If they don't add up to 1.0, dividing by their magnitued will
 * renormalize them.
 *
 * Note: See the following for more information on quaternions:
 *
 * - Shoemake, K., Animating rotation with quaternion curves, Computer
 *   Graphics 19, No 3 (Proc. SIGGRAPH'85), 245-254, 1985.
 * - Pletinckx, D., Quaternion calculus as a basic tool in computer
 *   graphics, The Visual Computer 5, 2-13, 1989.
 */
void normalize_quat(float q[4]) {
  int i;
  float mag;

  mag = (q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
  for (i = 0; i < 4; i++)
    q[i] /= mag;
}

/*
 * Build a rotation matrix, given a quaternion rotation.
 *
 */
void build_rotmatrix(float m[4][4], const float q[4]) {
  m[0][0] = 1.0 - 2.0 * (q[1] * q[1] + q[2] * q[2]);
  m[0][1] = 2.0 * (q[0] * q[1] - q[2] * q[3]);
  m[0][2] = 2.0 * (q[2] * q[0] + q[1] * q[3]);
  m[0][3] = 0.0;

  m[1][0] = 2.0 * (q[0] * q[1] + q[2] * q[3]);
  m[1][1] = 1.0 - 2.0 * (q[2] * q[2] + q[0] * q[0]);
  m[1][2] = 2.0 * (q[1] * q[2] - q[0] * q[3]);
  m[1][3] = 0.0;

  m[2][0] = 2.0 * (q[2] * q[0] - q[1] * q[3]);
  m[2][1] = 2.0 * (q[1] * q[2] + q[0] * q[3]);
  m[2][2] = 1.0 - 2.0 * (q[1] * q[1] + q[0] * q[0]);
  m[2][3] = 0.0;

  m[3][0] = 0.0;
  m[3][1] = 0.0;
  m[3][2] = 0.0;
  m[3][3] = 1.0;
}
