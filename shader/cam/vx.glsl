#version 330 core

// layout (location = 0) in vec4 pos;
layout (location = 0) in vec3 pos;
// layout (location = 1) in vec2 aTexCoord;

// out vec2 TexCoord;

// uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

void main() {
	gl_Position = projection * view * model * vec4(pos, 1);
	// gl_Position = pos;
	// TexCoord = vec2(aTexCoord.x, aTexCoord.y);
}
