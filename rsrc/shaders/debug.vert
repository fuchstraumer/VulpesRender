#version 450

#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 position;

layout(push_constant) uniform _ubo {
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(position, 1.0f);
}