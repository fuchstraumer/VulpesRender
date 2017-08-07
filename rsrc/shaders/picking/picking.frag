#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out vec4 outColor;
layout(location = 1) out uvec3 pickingData;

layout(push_constant) uniform push_data {
    layout(offset = 192) uvec2 Indices; // DrawIndex, ObjIndex
} pushData;

void main() {
    pickingData = uvec3(pushData.x, pushData.y, gl_PrimitiveID + 1);
}