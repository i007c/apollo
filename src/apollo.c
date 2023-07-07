
#include "apollo.h"
#include "logger.h"

#define LS SECTOR_MAIN_APOLLO

int WIDTH = 400;
int HEIGHT = 300;

// An array of 3 vectors which represents 3 vertices
// static const GLfloat g_vertex_buffer_data[] = {
//    -1.0f, -1.0f, 0.0f,
//    1.0f, -1.0f, 0.0f,
//    0.0f,  1.0f, 0.0f,
// };

void reshape(int width, int height) {
    log_verbose(
        "\033[34m{\033[0mreshape\033[34m}\033[0m %dx%d",
        width, height
    );

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 1, 0, 1);
}

void display(void) {
    log_verbose("\033[36m{\033[0mdisplay\033[36m}\033[0m");

	// Dark blue background
	glClearColor(0.4f, 0.4f, 0.4f, 0.0f);

	GLuint VertexArrayID =0;

	glGenVertexArrays(1, &VertexArrayID);

    log_verbose("vertex array id: %d", VertexArrayID);


	glBindVertexArray(VertexArrayID);



    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);

    // glClear(GL_COLOR_BUFFER_BIT);
    //
    // for (int i = 0; i < 20; i++) {
    //     glColor3f(randomFloat(), randomFloat(), randomFloat());
    //     glRasterPos3f(randomFloat(), randomFloat(), 0.0);
    //     glBitmap(27, 11, 0, 0, 0, 0, fish);
    // }
    //
    // glFlush();
}



void processInput(GLFWwindow *window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}


void framebuffer_size_callback(GLFWwindow* UNUSED(W), int width, int height) {
    glViewport(0, 0, width, height);
}

// int argc, char **argv
int main(void) {
    log_info(".. Apollo ..");

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "Apollo", NULL, NULL);
    if (window == NULL) {
        log_error("Failed to create GLFW window");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    // glutInit(&argc, argv);
    // glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
    // glutInitWindowSize(WIDTH, HEIGHT);
    // glutCreateWindow("Apollo");
    //
    // glutReshapeFunc(reshape);
    // glutDisplayFunc(display);
    // glutMainLoop();

    return 0;
}

