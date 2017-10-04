#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 position;

layout(push_constant) uniform UBO {
	mat4 view;
	mat4 projection;
} ubo;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec3 texPos;

void main(){
    vec4 pos = ubo.projection * ubo.view * vec4(position, 1.0f);
    // We use xyww to trick the depth test into always 
    // seeing the skybox as the absolute back-most object
    gl_Position = pos.xyww;
    texPos = position;
}