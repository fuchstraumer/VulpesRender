#include "vpr_stdafx.h"
#include <unordered_map>
#include "objects/ObjModel.hpp"
#include "core/Instance.hpp"
#include "command/TransferPool.hpp"
#include "BaseScene.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

namespace vulpes {

    void ObjModel::LoadModelFromFile(const std::string& obj_model_filename, TransferPool* transfer_pool) {
        
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string tinyobj_err;

        if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &tinyobj_err, obj_model_filename.c_str())) {
            LOG(ERROR) << "TinyObjLoader failed to load model file " << obj_model_filename << " , exiting.";
            LOG(ERROR) << "Load failed with error: " << tinyobj_err;
            throw std::runtime_error(tinyobj_err.c_str());
        }

        modelName = shapes.front().name;

        loadMeshes(shapes, attrib, transfer_pool);

        if(!materials.empty()) {
            createMaterials(materials, transfer_pool);
        }
        
    }

    void ObjModel::loadMeshes(const std::vector<tinyobj::shape_t>& shapes, const tinyobj::attrib_t& attrib, TransferPool* transfer_pool) {
       
        std::unordered_map<vertex_t, uint32_t> unique_vertices{};

        for (const auto& shape : shapes) {
            for (const auto& idx : shape.mesh.indices) {

                ReserveIndices(NumIndices() + shape.mesh.indices.size());
                vertex_t vert;
                vert.pos = { attrib.vertices[3 * idx.vertex_index], attrib.vertices[3 * idx.vertex_index + 1], attrib.vertices[3 * idx.vertex_index + 2] };
                vert.normal = { attrib.normals[3 * idx.normal_index], attrib.normals[3 * idx.normal_index + 1], attrib.normals[3 * idx.normal_index + 2] };
                vert.uv = { attrib.texcoords[2 * idx.texcoord_index], 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1] };

                if (unique_vertices.count(vert) == 0) {
                    unique_vertices[vert] = AddVertex(vert);
                }

                AddIndex(unique_vertices[vert]);
            }
        }

        CreateBuffers(device);
        LOG(INFO) << "Loaded mesh data from .obj file, uploading to device now...";
        auto& cmd = transfer_pool->Begin();
        RecordTransferCommands(cmd);
        
        transfer_pool->Submit();
        LOG(INFO) << "Mesh data upload complete.";

    }

    void ObjModel::createMaterials(const std::vector<tinyobj::material_t>& materials, TransferPool* transfer_pool) {
        for (const auto& material : materials) {
            if (materialsPool.count(material.name) == 0) {
                auto inserted = materialsPool.insert(std::make_pair(material.name, std::move(std::make_unique<Material>())));
                inserted.first->second->Create(material, device, descriptorPool);
                LOG(INFO) << "Created a new material object, sending to device now...";
                inserted.first->second->UploadToDevice(transfer_pool);
                LOG(INFO) << "New material object created and uploaded to device.";
                myMaterial = inserted.first;
            }
            else {
                myMaterial = materialsPool.find(material.name);
                if (myMaterial == materialsPool.cend()) {
                    LOG(ERROR) << "Couldn't find object's material in the materials pool!";
                    throw std::runtime_error("Couldn't find material belonging to object.");
                }
            }
        }
    }

    void ObjModel::CreateShaders(const std::string& vertex_shader_path, const std::string& fragment_shader_path) {

        vert = std::make_unique<ShaderModule>(device, vertex_shader_path.c_str(), VK_SHADER_STAGE_VERTEX_BIT);
        frag = std::make_unique<ShaderModule>(device, fragment_shader_path.c_str(), VK_SHADER_STAGE_FRAGMENT_BIT);

    }

    void ObjModel::createPipelineCache() {

        // can't use typeid, different models might have drastically different pipeline data due to textures etc
        pipelineCache = std::make_unique<PipelineCache>(device, static_cast<uint16_t>(std::hash<std::string>()(modelName)));

    }

    void ObjModel::createPipelineLayout() {

        std::vector<VkDescriptorSetLayout> set_layouts{
            myMaterial->second->GetSetLayout()
        };

        if (myMaterial->second->GetPbrSetLayout() != VK_NULL_HANDLE) {
            set_layouts.push_back(myMaterial->second->GetPbrSetLayout());
        }

        pipelineLayout = std::make_unique<PipelineLayout>(device);
        // since i'm already abusing this pipeline with the above sets, just using mvp in push constant
        pipelineLayout->Create(set_layouts, { VkPushConstantRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) } }); 

    }

    void ObjModel::setPipelineStateInfo() {

        constexpr static VkDynamicState dynamic_states[2]{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        pipelineStateInfo.DynamicStateInfo.dynamicStateCount = 2;
        pipelineStateInfo.DynamicStateInfo.pDynamicStates = dynamic_states;

        pipelineStateInfo.MultisampleInfo.sampleShadingEnable = BaseScene::SceneConfiguration.EnableMSAA;
        if (pipelineStateInfo.MultisampleInfo.sampleShadingEnable) {
            pipelineStateInfo.MultisampleInfo.rasterizationSamples = BaseScene::SceneConfiguration.MSAA_SampleCount;
        }

        pipelineStateInfo.VertexInfo.vertexBindingDescriptionCount = 1;
        pipelineStateInfo.VertexInfo.pVertexBindingDescriptions = &bindDescription;
        pipelineStateInfo.VertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        pipelineStateInfo.VertexInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    }

    void ObjModel::createGraphicsPipeline(const VkRenderPass& renderpass) {

        pipelineCreateInfo = pipelineStateInfo.GetPipelineCreateInfo();
        
        const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{
            vert->PipelineInfo(), frag->PipelineInfo()
        };

        pipelineCreateInfo.stageCount = static_cast<uint32_t>(shader_stages.size());
        pipelineCreateInfo.pStages = shader_stages.data();

        pipelineCreateInfo.layout = pipelineLayout->vkHandle();
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.renderPass = renderpass;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex = -1;

        graphicsPipeline = std::make_unique<GraphicsPipeline>(device);
        graphicsPipeline->Init(pipelineCreateInfo, pipelineCache->vkHandle());
        
    }

}