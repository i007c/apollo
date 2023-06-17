
#include "apollo.h"

#define LS SECTOR_MAIN_APOLLO

int WIDTH = 400;
int HEIGHT = 300;

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


int main(int argc, char **argv) {
    log_info(".. Apollo ..");

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Apollo");

    glutReshapeFunc(reshape);
    glutDisplayFunc(display);
    glutMainLoop();

    return 0;
}

