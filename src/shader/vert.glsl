#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 v_normal;

layout(set = 0, binding = 0) uniform Data {
    mat4 model;
    mat4 view;
    mat4 proj;
} uniforms;

void main() {
    // v_normal = transpose(inverse(mat3(worldview))) * normal;
    v_normal = transpose(inverse(mat3(uniforms.model))) * normal;

    mat4 worldview = uniforms.view * uniforms.model;
    gl_Position = uniforms.proj * worldview * vec4(position, 1.0);
}
