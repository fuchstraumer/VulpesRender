#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D textureSampler;

layout(location = 0) in vec2 vUV;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(textureSampler, vUV);
}