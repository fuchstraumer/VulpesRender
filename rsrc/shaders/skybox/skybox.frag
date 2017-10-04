#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 texPos;
layout(location = 0) out vec4 frag_color;
layout(set = 0, binding = 0) uniform samplerCube skybox;
void main(){
    frag_color = texture(skybox, texPos);
}