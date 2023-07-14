

#include "apollo.h"

#include "cglm/cglm.h"

#define LS SECTOR_MAIN_APOLLO
#define PI180 0.017453292519943295

// #define DONKEYOBJ_IMPLEMENTATION
// #include "donkeyobj.h"

static int width = 1080;
static int height = 720;

// void trackball(float q[4], float p1x, float p1y, float p2x, float p2y);
// void negate_quat(float *q, float *qn);
// void add_quats(vec4 q1, vec4 q2, vec4 dest);
//
// void build_rotmatrix(float m[4][4], const float q[4]);
// void axis_to_quat(float a[3], float phi, float q[4]);
// #define TRACKBALLSIZE (0.8)
// static float tb_project_to_sphere(float, float, float);
// static void normalize_quat(float[4]);

// static vec3 eye;
// static vec3 center;
// static vec3 up;

static bool mouse_left_pressed = false;
static bool mouse_middle_pressed = false;
static bool mouse_right_pressed = false;

// static bool mouse_init = false;

static float mouse_x_last = 0;
static float mouse_y_last = 0;

vec3 cam_position;
vec3 cam_front;
vec3 cam_up;
vec3 cam_right;
vec3 cam_world_up;
// euler Angles
float cam_yaw = -90;
float cam_pitch = 0;
// camera options
float cam_mouse_sensitivity = 0.0001;
float cam_zoom = 45;

// static vec4 curr_quat;
// static vec4 prev_quat;

vec3 cam_temp;

static void click_callback(GLFWwindow* window, int button, int action, int mods);
static void motion_callback(GLFWwindow* window, double mouse_x, double mouse_y);
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

void key_callback(
    GLFWwindow *win, int key,
    int UNUSED(scancode), int action, int UNUSED(code)
) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
            case GLFW_KEY_Q:
                glfwSetWindowShouldClose(win, true);
                break;

            case GLFW_KEY_W:
                // vec3 dest;
                // glm_vec3_scale(cam_front, 2.5, dest);
                glm_vec3_muladds(cam_front, 2.5, cam_position);
                log_verbose("pos: { %f, %f, %f }", cam_position[0], cam_position[1], cam_position[2]);
                break;
            case GLFW_KEY_S:
                glm_vec3_scale(cam_front, 2.5, cam_temp);
                glm_vec3_sub(cam_position, cam_temp, cam_position);
                log_verbose("pos: { %f, %f, %f }", cam_position[0], cam_position[1], cam_position[2]);
                break;

            case GLFW_KEY_D:
                glm_vec3_muladds(cam_right, 2.5, cam_position);

                log_verbose("pos: { %f, %f, %f }", cam_position[0], cam_position[1], cam_position[2]);
                break;
            case GLFW_KEY_A:
                glm_vec3_scale(cam_right, 2.5, cam_temp);
                glm_vec3_sub(cam_position, cam_temp, cam_position);

                log_verbose("pos: { %f, %f, %f }", cam_position[0], cam_position[1], cam_position[2]);
                break;

        // if (direction == FORWARD)
        //     Position += Front * velocity;
        // if (direction == BACKWARD)
        //     Position -= Front * velocity;
        // if (direction == LEFT)
        //     Position -= Right * velocity;
        // if (direction == RIGHT)
        //     Position += Right * velocity;

            // case GLFW_KEY_1:
            //     eye[2] += 0.1;
            //     if (eye[2] > 10) eye[2] = 10;
            //     break;
            // case GLFW_KEY_3:
            //     eye[2] -= 0.1;
            //     if (eye[2] < 1) eye[2] = 1;
            //     break;
            //
            // case GLFW_KEY_W:
            //     center[2] += 1;
            //     log_verbose("up: { %f, %f, %f }", up[0], up[1], up[2]);
            //     break;
            //
            // case GLFW_KEY_S:
            //     center[2] -= 1;
            //     log_verbose("up: { %f, %f, %f }", up[0], up[1], up[2]);
            //     // log_verbose("center: { %f, %f, %f }", center[0], center[1], center[2]);
            //     // log_verbose("eye: { %f, %f, %f }", eye[0], eye[1], eye[2]);
            //     break;
        }
    }
}


void framebuffer_size_callback(GLFWwindow* UNUSED(W), int w, int h) {
    width = w;
    height = h;
    glViewport(0, 0, width, height);
}

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

// int argc, char **argv
int main(void) {
    log_info("Starting Apollo");

    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(width, height, "Apollo", NULL, NULL);
    if (window == NULL) {
        log_error("Failed to create GLFW window");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwSwapInterval(1);

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, click_callback);
    glfwSetCursorPosCallback(window, motion_callback);
    glfwSetScrollCallback(window, scroll_callback);


    if (glewInit() != GLEW_OK) {
        log_error("glew init faild");
        return EXIT_FAILURE;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(debug_message, NULL);

    glEnable(GL_DEPTH_TEST);
    
    // trackball(curr_quat, 0, 0, 0, 0);
    //
    // eye[0] = 0.0f;
    // eye[1] = 0.0f;
    // eye[2] = 3.0f;
    //
    // center[0] = 0.0f;
    // center[1] = 0.0f;
    // center[2] = 0.0f;
    //
    // up[0] = 0.0f;
    // up[1] = 1.0f;
    // up[2] = 0.0f;


    cam_world_up[0] = 0;
    cam_world_up[1] = 1;
    cam_world_up[2] = 0;

    cam_position[0] = 0;
    cam_position[1] = 0;
    cam_position[2] = 3;

    cam_front[0] = 0;
    cam_front[1] = 0;
    cam_front[2] = -1;

    glm_vec3_zero(cam_right);

    cam_front[0] = cos(cam_yaw / PI180) * cos(cam_pitch / PI180);
    cam_front[1] = sin(cam_pitch / PI180);
    cam_front[2] = sin(cam_yaw / PI180) * cos(cam_pitch / PI180);

    glm_vec3_normalize(cam_front);

    glm_vec3_cross(cam_front, cam_world_up, cam_right);
    glm_vec3_normalize(cam_right);

    glm_vec3_cross(cam_right, cam_front, cam_up);
    glm_vec3_normalize(cam_up);


    // donkey_ctx ctx;
    // donkeyobj("object/cow.obj", &ctx);

    // GLuint vertexbuffer;
    // glGenBuffers(1, &vertexbuffer);
    // glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    //
    // GLuint colorbuffer;
    // glGenBuffers(1, &colorbuffer);
    // glBindBuffer(GL_ARRAY_BUFFER, colorbuffer);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(g_color_buffer_data), g_color_buffer_data, GL_STATIC_DRAW);

    float cube[] = {
        1.000000, 1.000000, -1.000000,
        1.000000, -1.000000, -1.000000,
        1.000000, 1.000000, 1.000000,
        1.000000, -1.000000, 1.000000,
        -1.000000, 1.000000, -1.000000,
        -1.000000, -1.000000, -1.000000,
        -1.000000, 1.000000, 1.000000,
        -1.000000, -1.000000, 1.000000,
    };

    uint32_t ind[] = {
        0, 4, 6,
        0, 6, 2,
        3, 2, 6,
        3, 6, 7,
        7, 6, 4,
        7, 4, 5,
        5, 1, 3,
        5, 3, 7,
        1, 0, 2,
        1, 2, 3,
        5, 4, 0,
        5, 0, 1,
    };

    GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

    uint32_t vbuf = 0;
    glGenBuffers(1, &vbuf);
    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(pos), pos, GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_TRUE, 0, 0);

    uint32_t ibuf = 0;
    glGenBuffers(1, &ibuf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ind), ind, GL_STATIC_DRAW);


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

        mat4 projection;
        glm_mat4_identity(projection);
        // glm_ortho(0, width, height, 0, -1, 1, projection);
        glm_perspective(cam_zoom/PI180, width/height, 0.1, 100, projection);
        glUniformMatrix4fv(uloc_projection, 1, GL_FALSE, &projection[0][0]);

        vec3 cam_pf;
        glm_vec3_add(cam_position, cam_front, cam_pf);

        mat4 view;
        glm_lookat(cam_position, cam_pf, cam_up, view);
        glUniformMatrix4fv(uloc_view, 1, GL_FALSE, &view[0][0]);

        mat4 model = {
            { 1.0f, 0.0f, 0.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 1.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f, 1.0f },
        };
        // glm_mat4_identity(model);
        glUniformMatrix4fv(uloc_model, 1, GL_FALSE, &model[0][0]);

        // mat4 projection;
        // glm_perspective(45/PI180, width/height, 0.1, 100, projection);
        // glUniformMatrix4fv(uloc_projection, 1, GL_FALSE, &projection[0][0]);
        //
        // mat4 view;
        // glm_lookat(eye, center, up, view);
        // glUniformMatrix4fv(uloc_view, 1, GL_FALSE, &view[0][0]);
        //
        // mat4 tb;
        // build_rotmatrix(tb, curr_quat);
        // glUniformMatrix4fv(uloc_tb, 1, GL_FALSE, &tb[0][0]);


        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glUniform4f(ucloc, 0.32, 0.6, 1, 1);

        // glDrawArrays(GL_TRIANGLES, 0, LEN(cube) / 3);
        glDrawElements(GL_TRIANGLES, LEN(ind), GL_UNSIGNED_INT, 0);

        glLineWidth(4);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glUniform4f(ucloc, 0, 0, 0, 1);

        // glDrawArrays(GL_TRIANGLES, 0, LEN(cube) / 3);
        glDrawElements(GL_TRIANGLES, LEN(ind), GL_UNSIGNED_INT, 0);


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



static void motion_callback(GLFWwindow* UNUSED(W), double mouse_x, double mouse_y) {

    if (!mouse_left_pressed) {
        mouse_x_last = mouse_x;
        mouse_y_last = mouse_y;
        return;
    }

    float offset_x = mouse_x - mouse_x_last;
    float offset_y = mouse_y_last - mouse_y;

    mouse_x_last = mouse_x;
    mouse_y_last = mouse_y;

    offset_x *= cam_mouse_sensitivity;
    offset_y *= cam_mouse_sensitivity;

    cam_yaw += offset_x;
    cam_pitch += offset_y;

    if (cam_pitch > 89.0f)
        cam_pitch = 89.0f;

    if (cam_pitch < -89.0f)
        cam_pitch = -89.0f;

    cam_front[0] = cos(cam_yaw / PI180) * cos(cam_pitch / PI180);
    cam_front[1] = sin(cam_pitch / PI180);
    cam_front[2] = sin(cam_yaw / PI180) * cos(cam_pitch / PI180);

    glm_vec3_normalize(cam_front);

    glm_vec3_cross(cam_front, cam_world_up, cam_right);
    glm_vec3_normalize(cam_right);

    glm_vec3_cross(cam_right, cam_front, cam_up);
    glm_vec3_normalize(cam_up);

    // return;
    // float rotScale = 1.0f;
    // float transScale = 2.0f;
    //
    // if (mouse_left_pressed) {
    //     trackball(prev_quat, rotScale * (2.0f * mouse_x_last - width) / (float)width,
    //               rotScale * (height - 2.0f * mouse_y_last) / (float)height,
    //               rotScale * (2.0f * (float)mouse_x - width) / (float)width,
    //               rotScale * (height - 2.0f * (float)mouse_y) / (float)height);
    //
    //     add_quats(prev_quat, curr_quat, curr_quat);
    // } else if (mouse_middle_pressed) {
    //     eye[0] -= transScale * ((float)mouse_x - mouse_x_last) / (float)width;
    //     center[0] -= transScale * ((float)mouse_x - mouse_x_last) / (float)width;
    //     eye[1] += transScale * ((float)mouse_y - mouse_y_last) / (float)height;
    //     center[1] += transScale * ((float)mouse_y - mouse_y_last) / (float)height;
    // } else if (mouse_right_pressed) {
    //     eye[2] += transScale * ((float)mouse_y - mouse_y_last) / (float)height;
    //     center[2] += transScale * ((float)mouse_y - mouse_y_last) / (float)height;
    // }
    //
    // mouse_x_last = mouse_x;
    // mouse_y_last = mouse_y;
}


static void click_callback(
    GLFWwindow* UNUSED(W), int button, int action, int UNUSED(M)
) {

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mouse_left_pressed = true;
            // trackball(prev_quat, 0.0, 0.0, 0.0, 0.0);
        } else if (action == GLFW_RELEASE) {
            mouse_left_pressed = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            mouse_right_pressed = true;
        } else if (action == GLFW_RELEASE) {
            mouse_right_pressed = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            mouse_middle_pressed = true;
        } else if (action == GLFW_RELEASE) {
            mouse_middle_pressed = false;
        }
    }
}

static void scroll_callback(
    GLFWwindow *UNUSED(W), double UNUSED(X), double yoffset
) {
    cam_zoom -= (float)yoffset;

    if (cam_zoom < 1.0f)
        cam_zoom = 1.0f;

    if (cam_zoom > 45.0f)
        cam_zoom = 45.0f;
}

