#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform push_constant_data {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 vUV;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(position, 1.0f);
    vUV = uv;
}