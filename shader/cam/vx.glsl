#version 330 core

// layout (location = 0) in vec4 pos;
layout (location = 0) in vec3 pos;
// layout (location = 1) in vec2 aTexCoord;

// out vec2 TexCoord;

out vec4 vx_color;

// uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

void main() {
	gl_Position = projection * view * model * vec4(pos, 1);
    float red = 0.5 * (pos.x + 1.0);
    vx_color = vec4(red, 0.5 * (pos.y + 1.0), 0.5 * (pos.z + 1.0), 1);
	// gl_Position = pos;
	// TexCoord = vec2(aTexCoord.x, aTexCoord.y);
}
