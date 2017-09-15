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

    public:

        Material() = default;
        ~Material();

        void Create(const std::string& mtl_file_path, const Device* device, DescriptorPool* descriptor_pool);
        void Create(const tinyobj::material_t& tinyobj_imported_material, const Device* dvc, DescriptorPool* descriptor_pool);
        void UploadToDevice(TransferPool* transfer_pool);


    private:

        void createUBO(const tinyobj::material_t& material_);
        void createTextures(const tinyobj::material_t& material_);
    
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
        ~pbrTexturePack();

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