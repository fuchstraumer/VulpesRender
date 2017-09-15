#include "vpr_stdafx.h"
#include "objects/Materials.hpp"
#include "core/LogicalDevice.hpp"

namespace vulpes {

    void Material::Create(const tinyobj::material_t& material_, const Device* device, DescriptorPool* descriptor_pool) {

        createUBO(material_);
        createTextures(material_);
        descriptorSet->Init(descriptor_pool);
        
    }

    void Material::createUBO(const tinyobj::material_t& material_) {

        uboData.ambient = glm::vec4(material_.ambient[0], material_.ambient[1], material_.ambient[2], 1.0f);
        uboData.diffuse = glm::vec4(material_.diffuse[0], material_.diffuse[1], material_.diffuse[2], 1.0f);
        uboData.specular = glm::vec4(material_.specular[0], material_.specular[1], material_.specular[2], 1.0f);
        uboData.transmittance = glm::vec4(material_.transmittance[0], material_.transmittance[1], material_.transmittance[2], 1.0f);
        uboData.emission = glm::vec4(material_.emission[0], material_.emission[1], material_.emission[2], 1.0f);
        uboData.miscInfo.shininess = material_.shininess;
        uboData.miscInfo.indexOfRefraction = material_.ior;
        uboData.miscInfo.shininess = material_.shininess;
        uboData.miscInfo.illuminationModel = material_.illum;

        ubo = std::make_unique<Buffer>(device);
        ubo->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(uboData));
        descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);
        descriptorSet->AddDescriptorInfo(ubo->GetDescriptor(), 0);

    }

    void Material::createTextures(const tinyobj::material_t& material_) {

        uint32_t curr_binding_idx = 1; // spot 0 is always uniform buffer.

        if(!material_.ambient_texname.empty()) {
            ambient = std::make_unique<Texture<texture_2d_t>>(device);
            ambient->CreateFromFile(material_.ambient_texname.c_str());

            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(ambient->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.diffuse_texname.empty()) {
            diffuse = std::make_unique<Texture<texture_2d_t>>(device);
            diffuse->CreateFromFile(material_.diffuse_texname.c_str());

            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(diffuse->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.specular_texname.empty()) {
            specular = std::make_unique<Texture<texture_2d_t>>(device);
            specular->CreateFromFile(material_.specular_texname.c_str());

            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(specular->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.specular_highlight_texname.empty()) {
            specularHighlight = std::make_unique<Texture<texture_2d_t>>(device);
            specularHighlight->CreateFromFile(material_.specular_highlight_texname.c_str());

            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(specularHighlight->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.bump_texname.empty()) {
            bumpMap = std::make_unique<Texture<texture_2d_t>>(device);
            bumpMap->CreateFromFile(material_.bump_texname.c_str());

            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(bumpMap->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.displacement_texname.empty()) {
            displacementMap = std::make_unique<Texture<texture_2d_t>>(device);
            displacementMap->CreateFromFile(material_.displacement_texname.c_str());

            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(displacementMap->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.alpha_texname.empty()) {
            alpha = std::make_unique<Texture<texture_2d_t>>(device);
            alpha->CreateFromFile(material_.alpha_texname.c_str());

            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(alpha->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.reflection_texname.empty()) {
            reflection = std::make_unique<Texture<texture_2d_t>>(device);
            reflection->CreateFromFile(material_.reflection_texname.c_str());

            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(reflection->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

    }
}