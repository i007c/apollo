
#include <GLFW/glfw3.h>

#include "apollo.h"
#include "logger.h"
#define LS SECTOR_MAIN_WINDOW

#include <stdbool.h>

extern State state;

static void click_callback(GLFWwindow* win, int button, int action, int mods);
static void motion_callback(GLFWwindow* win, double mouse_x, double mouse_y);
static void scroll_callback(GLFWwindow* win, double xoffset, double yoffset);
static void framebuffer_size_callback(GLFWwindow* win, int w, int h);
static void key_callback(
    GLFWwindow *win, int key, int scancode, int action, int code
);

status_t window_init(GLFWwindow **win) {
    log_info("window init");

    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    *win = glfwCreateWindow(
        state.width, state.height,
        "Apollo", NULL, NULL
    );
    GLFWwindow *window = *win;

    if (window == NULL) {
        log_error("Failed to create GLFW window");
        glfwTerminate();
        return STS_WIN_ERR;
    }

    glfwSwapInterval(1);

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, click_callback);
    glfwSetCursorPosCallback(window, motion_callback);
    glfwSetScrollCallback(window, scroll_callback);

    return STS_SUCCESS;
}

void framebuffer_size_callback(GLFWwindow* UNUSED(W), int w, int h) {
    state.width = w;
    state.height = h;
    glViewport(0, 0, w, h);
}


void key_callback(GLFWwindow *win, int key, int UNUSED(scancode), int action, int UNUSED(code)) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
            case GLFW_KEY_Q:
                glfwSetWindowShouldClose(win, true);
                break;

            case GLFW_KEY_W:
                state.eye[1] -= state.move_speed / state.height;
                state.center[1] -= state.move_speed / state.height;
                break;
            case GLFW_KEY_S:
                state.eye[1] += state.move_speed / state.height;
                state.center[1] += state.move_speed / state.height;
                break;

            case GLFW_KEY_A:
                state.eye[0] += state.move_speed / state.width;
                state.center[0] += state.move_speed / state.width;
                break;
            case GLFW_KEY_D:
                state.eye[0] -= state.move_speed / state.width;
                state.center[0] -= state.move_speed / state.width;
                break;

            case GLFW_KEY_T:
                state.wireframe = !state.wireframe;
                break;

            // case GLFW_KEY_L:
            //     cam_logger();
            //     break;
            //
            // case GLFW_KEY_W:
            //     glm_vec3_muladds(cam_front, cam_key_sensitivity, cam_position);
            //     break;
            //
            // case GLFW_KEY_S:
            //     glm_vec3_scale(cam_front, cam_key_sensitivity, cam_temp);
            //     glm_vec3_sub(cam_position, cam_temp, cam_position);
            //     break;
            //
            // case GLFW_KEY_D:
            //     glm_vec3_muladds(cam_right, cam_key_sensitivity, cam_position);
            //     break;
            //
            // case GLFW_KEY_A:
            //     glm_vec3_scale(cam_right, cam_key_sensitivity, cam_temp);
            //     glm_vec3_sub(cam_position, cam_temp, cam_position);
            //     break;

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


static void click_callback(
    GLFWwindow* UNUSED(W), int button, int action, int UNUSED(M)
) {

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            state.mouse_left_pressed = true;
            trackball(state.prev_quat, 0.0, 0.0, 0.0, 0.0);
        } else if (action == GLFW_RELEASE) {
            state.mouse_left_pressed = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            state.mouse_right_pressed = true;
        } else if (action == GLFW_RELEASE) {
            state.mouse_right_pressed = false;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            state.mouse_middle_pressed = true;
        } else if (action == GLFW_RELEASE) {
            state.mouse_middle_pressed = false;
        }
    }
}

static void scroll_callback(
    GLFWwindow *UNUSED(W), double UNUSED(X), double yoffset
) {
    // log_verbose("yoff: %f", yoffset);
    state.zoom -= (float)yoffset;
    // cam_fov -= (float)yoffset;
    //
    // if (cam_fov < 1.0f)
    //     cam_fov = 1.0f;
    //
    // if (cam_fov > 45.0f)
    //     cam_fov = 45.0f;
}

static void motion_callback(GLFWwindow* UNUSED(W), double mouse_x, double mouse_y) {

    // if (!mouse_left_pressed) {
    //     mouse_x_last = mouse_x;
    //     mouse_y_last = mouse_y;
    //     return;
    // }
    //
    // float offset_x = mouse_x - mouse_x_last;
    // float offset_y = mouse_y_last - mouse_y;
    //
    // mouse_x_last = mouse_x;
    // mouse_y_last = mouse_y;
    //
    // offset_x *= cam_mouse_sensitivity;
    // offset_y *= cam_mouse_sensitivity;
    //
    // cam_yaw += offset_x;
    // cam_pitch += offset_y;
    //
    // if (cam_pitch > 89.0f)
    //     cam_pitch = 89.0f;
    //
    // if (cam_pitch < -89.0f)
    //     cam_pitch = -89.0f;
    //
    // cam_front[0] = cos(cam_yaw / PI180) * cos(cam_pitch / PI180);
    // cam_front[1] = sin(cam_pitch / PI180);
    // cam_front[2] = sin(cam_yaw / PI180) * cos(cam_pitch / PI180);
    //
    // glm_vec3_normalize(cam_front);
    //
    // glm_vec3_cross(cam_front, cam_world_up, cam_right);
    // glm_vec3_normalize(cam_right);
    //
    // glm_vec3_cross(cam_right, cam_front, cam_up);
    // glm_vec3_normalize(cam_up);

    float rotScale = 1.0f;
    float transScale = 2.0f;

    if (state.mouse_left_pressed) {
        trackball(state.prev_quat, rotScale * (2.0f * state.mouse_x_last - state.width) / (float)state.width,
                  rotScale * (state.height - 2.0f * state.mouse_y_last) / (float)state.height,
                  rotScale * (2.0f * (float)mouse_x - state.width) / (float)state.width,
                  rotScale * (state.height - 2.0f * (float)mouse_y) / (float)state.height);

        add_quats(state.prev_quat, state.curr_quat, state.curr_quat);
    } else if (state.mouse_middle_pressed) {
       state.eye[0] -= transScale * ((float)mouse_x - state.mouse_x_last) / (float)state.width;
       state.center[0] -= transScale * ((float)mouse_x - state.mouse_x_last) / (float)state.width;
       state.eye[1] += transScale * ((float)mouse_y - state.mouse_y_last) / (float)state.height;
       state.center[1] += transScale * ((float)mouse_y - state.mouse_y_last) / (float)state.height;
    } else if (state.mouse_right_pressed) {
        state.eye[2] += transScale * ((float)mouse_y - state.mouse_y_last) / (float)state.height;
        state.center[2] += transScale * ((float)mouse_y - state.mouse_y_last) / (float)state.height;
    }

    state.mouse_x_last = mouse_x;
    state.mouse_y_last = mouse_y;
}
