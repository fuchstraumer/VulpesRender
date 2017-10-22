#pragma once
#ifndef VULPESRENDER_MATERIALS_HPP
#define VULPESRENDER_MATERIALS_HPP

#include "vpr_stdafx.h"
#include "resource/Texture.hpp"
#include "resource/DescriptorSet.hpp"
#include "resource/Buffer.hpp"
#include "tinyobj/tiny_obj_loader.h"

namespace vulpes {

    struct pbrTexturePack;

    /** Hides away the tremendous amount of textures and data required to render more complicated .obj models.
    *   Uses tinyobj to load the obj file, and enables all of the features it can. Can also be passed an mtl file path,
    *   otherwise accepts a tinyobj::material_t reference that it will use for loading the textures.
    *   
    *   When available, it will also load and create the textures required for PBR rendering. Note that shaders must still
    *   be setup to use the available textures, and a descriptor set with enough VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER objects
    *   allocated must also exist (and be passed to the Create method).
    *   \ingroup Objects
    */
    class Material {
    
        struct material_ubo_t {
            glm::vec4 ambient;
            glm::vec4 diffuse;
            glm::vec4 specular;
            glm::vec4 transmittance;
            glm::vec4 emission;
            struct misc_info_t {
                float shininess;
                float indexOfRefraction;
                float dissolve;
                int illuminationModel;
            } miscInfo;
        };

        Material(const Material&) = delete;
        Material& operator=(const Material&) = delete;

    public:

        Material() = default;
        ~Material() = default;

        Material(Material&&) noexcept;
        Material& operator=(Material&&) noexcept;

        void Create(const std::string& mtl_file_path, const Device* device, DescriptorPool* descriptor_pool);
        void Create(const tinyobj::material_t& tinyobj_imported_material, const Device* dvc, DescriptorPool* descriptor_pool);
        void UploadToDevice(TransferPool* transfer_pool);

        /** Binds the descriptor set belonging to this object, to which all available textures will have been attached.
        *   This is the only required command during rendering to use the Material class.
        */
        void BindMaterial(const VkCommandBuffer& cmd, const VkPipelineLayout& pipeline_layout) const noexcept;

        VkDescriptorSetLayout GetSetLayout() const noexcept;
        VkDescriptorSetLayout GetPbrSetLayout() const noexcept;

    private:

        void createUBO(const tinyobj::material_t& material_);
        void createTextures(const tinyobj::material_t& material_);
        void createPbrTexturePack(const tinyobj::material_t& material_, const uint32_t& curr_binding_idx);
    
        std::unique_ptr<Texture<texture_2d_t>> ambient;
        std::unique_ptr<Texture<texture_2d_t>> diffuse;
        std::unique_ptr<Texture<texture_2d_t>> specular;
        std::unique_ptr<Texture<texture_2d_t>> specularHighlight;
        std::unique_ptr<Texture<texture_2d_t>> bumpMap;
        std::unique_ptr<Texture<texture_2d_t>> displacementMap;
        std::unique_ptr<Texture<texture_2d_t>> alpha;
        std::unique_ptr<Texture<texture_2d_t>> reflection;

        std::unique_ptr<Buffer> ubo;
        material_ubo_t uboData;
        std::unique_ptr<DescriptorSet> descriptorSet;
        // OPTIONAL
        std::unique_ptr<pbrTexturePack> pbrTextures;

        const Device* device;

        std::list<Texture<texture_2d_t>*> activeTextures;

    };

    struct pbrTexturePack {

        struct pbr_ubo_t {
            float roughness;
            float metallic;
            float sheen;
            float clearcoat_thickness;
            float clearcoat_roughness;
            float anisotropy;
            float anisotropy_rotation;
            const char padding[4]{ 0, 0, 0, 0 };
        };

        pbrTexturePack() = default;
        ~pbrTexturePack() = default;

        std::unique_ptr<Texture<texture_2d_t>> Roughness;
        std::unique_ptr<Texture<texture_2d_t>> Metallic;
        std::unique_ptr<Texture<texture_2d_t>> Sheen;
        std::unique_ptr<Texture<texture_2d_t>> Emissive;
        std::unique_ptr<Texture<texture_2d_t>> NormalMap;
        std::unique_ptr<Buffer> ubo;
        pbr_ubo_t uboData;

        std::unique_ptr<DescriptorSet> descriptorSet;

    };

}

#endif //!VULPESRENDER_MATERIALS_HPP