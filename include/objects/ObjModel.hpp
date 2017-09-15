#ifndef VULPESRENDER_OBJ_MODEL_HPP
#define VULPESRENDER_OBJ_MODEL_HPP

#include "vpr_stdafx.h"
#include "TriangleMesh.hpp"
#include "resource/Texture.hpp"
#include "resource/Buffer.hpp"
#include "resource/ShaderModule.hpp"
#include "resource/DescriptorSet.hpp"
#include "resource/PipelineLayout.hpp"
#include "resource/PipelineCache.hpp"
#include "render/GraphicsPipeline.hpp"
#include "tinyobj/tiny_obj_loader.h"
#include "Materials.hpp"

namespace vulpes {

    struct obj_model_ubo_t {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 cameraPosition;
    };

    class ObjModel {
        ObjModel(const ObjModel&) = delete;
        ObjModel& operator=(const ObjModel&) = delete;
    public:

        ObjModel(const Device* dvc);
        ~ObjModel();

        void LoadModelFromFile(const std::string& obj_model_filename);
        void UpdateUBO(const glm::mat4& view);

    private:

        void createMaterials(const std::vector<tinyobj::material_t>& materials);
        void createBuffers();
        void createTexture();
        void createResources(TransferPool* transfer_pool);
        void uploadResources(TransferPool* transfer_pool);

        void createShaders();
        void createPipelineCache();
        void createDescriptorSet(DescriptorPool* descriptor_pool);
        void createPipelineLayout();
        void setPipelineStateInfo();
        void createGraphicsPipeline();

        constexpr static VkVertexInputBindingDescription bindDescription {
            0, sizeof(vertex_t), VK_VERTEX_INPUT_RATE_VERTEX
        };

        constexpr static std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions {
            VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
            VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(glm::vec3) },
            VkVertexInputAttributeDescription{ 2, 0, VK_FORMAT_R32G32_SFLOAT, 2 * sizeof(glm::vec3) }
        };

        std::vector<std::unique_ptr<Material>> materials;
        std::unique_ptr<ShaderModule> vert, frag;
        std::unique_ptr<PipelineLayout> pipelineLayout;
        std::unique_ptr<PipelineCache> pipelineCache;
        std::unique_ptr<GraphicsPipeline> graphicsPipeline;

        GraphicsPipelineInfo pipelineStateInfo;
        VkGraphicsPipelineCreateInfo pipelineCreateInfo;
    };

}


#endif //!VULPESRENDER_OBJ_MODEL_HPP