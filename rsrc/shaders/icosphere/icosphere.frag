#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNorm;
layout(location = 3) in vec2 vUV;

layout(push_constant) uniform _fragment_push_data {
    layout(offset = 192) vec4 lightPosition;
    layout(offset = 208) vec4 viewerPosition;
    layout(offset = 224) vec4 lightColor;
} ubo;

layout(set = 0, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 fragColor;

void main() {
    
    const float ambient_strength = 0.3f;
    vec3 ambient = ambient_strength * ubo.lightColor.xyz;

    const float diffuse_strength = 0.6f;
    vec3 light_direction = normalize(ubo.lightPosition.xyz - vPos);
    float diff = max(dot(vNorm, light_direction), 0.0f);
    vec3 diffuse = diffuse_strength * diff * ubo.lightColor.xyz;

    float specular_strength = 0.3f;
    vec3 view_direction = normalize(ubo.viewerPosition.xyz - vPos);
    vec3 reflect_direction = reflect(-light_direction, vNorm);
    float spec = pow(max(dot(view_direction, reflect_direction), 0.0f), 32.0f);
    vec3 specular = specular_strength * spec * ubo.lightColor.xyz;

    vec4 light_result = vec4(ambient + diffuse + specular, 1.0f);
    fragColor = texture(textureSampler, vUV);
    fragColor *= light_result;
    fragColor.a = 1.0f;

}