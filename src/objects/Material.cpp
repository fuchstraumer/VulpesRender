#include "vpr_stdafx.h"
#include "objects/Material.hpp"
#include "core/LogicalDevice.hpp"
#include "command/TransferPool.hpp"

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
            activeTextures.push_back(ambient.get());
            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(ambient->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.diffuse_texname.empty()) {
            diffuse = std::make_unique<Texture<texture_2d_t>>(device);
            diffuse->CreateFromFile(material_.diffuse_texname.c_str());
            activeTextures.push_back(diffuse.get());
            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(diffuse->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.specular_texname.empty()) {
            specular = std::make_unique<Texture<texture_2d_t>>(device);
            specular->CreateFromFile(material_.specular_texname.c_str());
            activeTextures.push_back(specular.get());
            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(specular->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.specular_highlight_texname.empty()) {
            specularHighlight = std::make_unique<Texture<texture_2d_t>>(device);
            specularHighlight->CreateFromFile(material_.specular_highlight_texname.c_str());
            activeTextures.push_back(specularHighlight.get());
            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(specularHighlight->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.bump_texname.empty()) {
            bumpMap = std::make_unique<Texture<texture_2d_t>>(device);
            bumpMap->CreateFromFile(material_.bump_texname.c_str());
            activeTextures.push_back(bumpMap.get());
            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(bumpMap->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.displacement_texname.empty()) {
            displacementMap = std::make_unique<Texture<texture_2d_t>>(device);
            displacementMap->CreateFromFile(material_.displacement_texname.c_str());
            activeTextures.push_back(displacementMap.get());
            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(displacementMap->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.alpha_texname.empty()) {
            alpha = std::make_unique<Texture<texture_2d_t>>(device);
            alpha->CreateFromFile(material_.alpha_texname.c_str());
            activeTextures.push_back(alpha.get());
            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(alpha->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        if(!material_.reflection_texname.empty()) {
            reflection = std::make_unique<Texture<texture_2d_t>>(device);
            reflection->CreateFromFile(material_.reflection_texname.c_str());
            activeTextures.push_back(reflection.get());
            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(reflection->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;
        }

        createPbrTexturePack(material_, curr_binding_idx);

    }

    void Material::createPbrTexturePack(const tinyobj::material_t& material_, const uint32_t& last_binding_idx) {

        uint32_t curr_binding_idx = last_binding_idx;

        if(!material_.roughness_texname.empty() || !material_.metallic_texname.empty() || !material_.sheen_texname.empty() || !material_.emissive_texname.empty() || !material_.normal_texname.empty()) {
            
            pbrTextures = std::make_unique<pbrTexturePack>();

            pbrTextures->uboData.roughness = material_.roughness;
            pbrTextures->uboData.metallic = material_.metallic;
            pbrTextures->uboData.sheen = material_.sheen;
            pbrTextures->uboData.clearcoat_thickness = material_.clearcoat_thickness;
            pbrTextures->uboData.clearcoat_roughness = material_.clearcoat_roughness;
            pbrTextures->uboData.anisotropy = material_.anisotropy;
            pbrTextures->uboData.anisotropy_rotation = material_.anisotropy;
            pbrTextures->ubo = std::make_unique<Buffer>(device);
            pbrTextures->ubo->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(pbrTextures->uboData));
            descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
            descriptorSet->AddDescriptorInfo(pbrTextures->ubo->GetDescriptor(), curr_binding_idx);
            ++curr_binding_idx;

            if(!material_.roughness_texname.empty()) {
                pbrTextures->Roughness = std::make_unique<Texture<texture_2d_t>>(device);
                pbrTextures->Roughness->CreateFromFile(material_.roughness_texname.c_str());
                activeTextures.push_back(pbrTextures->Roughness.get());
                descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
                descriptorSet->AddDescriptorInfo(pbrTextures->Roughness->GetDescriptor(), curr_binding_idx);
                ++curr_binding_idx;
            }

            if(!material_.metallic_texname.empty()) {
                pbrTextures->Metallic = std::make_unique<Texture<texture_2d_t>>(device);
                pbrTextures->Metallic->CreateFromFile(material_.metallic_texname.c_str());
                activeTextures.push_back(pbrTextures->Metallic.get());
                descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
                descriptorSet->AddDescriptorInfo(pbrTextures->Metallic->GetDescriptor(), curr_binding_idx);
                ++curr_binding_idx;
            }

            if(!material_.sheen_texname.empty()) {
                pbrTextures->Sheen = std::make_unique<Texture<texture_2d_t>>(device);
                pbrTextures->Sheen->CreateFromFile(material_.metallic_texname.c_str());
                activeTextures.push_back(pbrTextures->Sheen.get());
                descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
                descriptorSet->AddDescriptorInfo(pbrTextures->Sheen->GetDescriptor(), curr_binding_idx);
                ++curr_binding_idx;
            }

            if(!material_.emissive_texname.empty()){
                pbrTextures->Emissive = std::make_unique<Texture<texture_2d_t>>(device);
                pbrTextures->Emissive->CreateFromFile(material_.emissive_texname.c_str());
                activeTextures.push_back(pbrTextures->Emissive.get());
                descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
                descriptorSet->AddDescriptorInfo(pbrTextures->Emissive->GetDescriptor(), curr_binding_idx);
                ++curr_binding_idx;
            }

            if(!material_.normal_texname.empty()) {
                pbrTextures->NormalMap = std::make_unique<Texture<texture_2d_t>>(device);
                pbrTextures->NormalMap->CreateFromFile(material_.normal_texname.c_str());
                activeTextures.push_back(pbrTextures->NormalMap.get());
                descriptorSet->AddDescriptorBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, curr_binding_idx);
                descriptorSet->AddDescriptorInfo(pbrTextures->NormalMap->GetDescriptor(), curr_binding_idx);
                ++curr_binding_idx;
            }
            
        }
    }

    void Material::UploadToDevice(TransferPool* transfer_pool) {
        
        std::mutex transfer_mutex;
        std::lock_guard<std::mutex> transfer_guard(transfer_mutex);

        auto& cmd = transfer_pool->Begin();

        while(!activeTextures.empty()) {
            auto& texture_to_transfer = activeTextures.front();
            activeTextures.pop_front();
            texture_to_transfer->TransferToDevice(cmd);
        }

        ubo->CopyTo(&uboData, cmd, sizeof(uboData), 0);

        if(pbrTextures) {
            pbrTextures->ubo->CopyTo(&pbrTextures->uboData, cmd, sizeof(pbrTextures->uboData), 0);
        }

        transfer_pool->End();
        transfer_pool->Submit();

    }

    void Material::BindMaterial(const VkCommandBuffer & cmd, const VkPipelineLayout& pipeline_layout) const noexcept {
        std::vector<VkDescriptorSet> descriptor_sets;
        descriptor_sets.push_back(descriptorSet->vkHandle());
        if (pbrTextures) {
            descriptor_sets.push_back(pbrTextures->descriptorSet->vkHandle());
        }
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data(), 0, nullptr);
    }

    VkDescriptorSetLayout Material::GetSetLayout() const noexcept {
        return descriptorSet->vkLayout();
    }

    VkDescriptorSetLayout Material::GetPbrSetLayout() const noexcept {
        if (pbrTextures) {
            return pbrTextures->descriptorSet->vkLayout();
        }
        else {
            return VK_NULL_HANDLE;
        }
    }
}