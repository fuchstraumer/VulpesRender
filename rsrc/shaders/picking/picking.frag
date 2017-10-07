#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) out vec4 outColor;
layout(location = 1) out uint pickingData;

layout(push_constant) uniform push_data {
    layout(offset = 192) uint uuid; // DrawIndex, ObjIndex
} pushData;

void main() {
    outColor = vec4(0.0f);
    pickingData = pushData.uuid;
}