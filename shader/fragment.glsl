#version 330 core

out vec4 color;

in vec4 vx_color;

uniform vec4 u_color;
uniform float u_time;
uniform bool wireframe;

void main() {
    color = vec4(abs(sin(u_time)), 0, 0, 1);
    // u_color[0] = 0.3;
    // if (wireframe) {
    // 	color = vec4(0, 0, 0, 1);
    // } else {
    //     color = vx_color;
    // }
}
