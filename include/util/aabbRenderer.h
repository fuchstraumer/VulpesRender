#pragma once 
#ifndef VULPES_VK_AABB_RENDERER_H
#define VULPES_VK_AABB_RENDERER_H

#include "vpr_stdafx.h"
#include "AABB.h"
#include "ForwardDecl.h"
#include "../resource/Buffer.h"
#include "../render/GraphicsPipeline.h"
#include "../resource/PipelineCache.h"
#include "../resource/ShaderModule.h"
#include "../resource/PipelineLayout.h"

namespace vulpes {

    namespace util {

        class aabbRenderer {
            aabbRenderer(const aabbRenderer&) = delete;
            aabbRenderer& operator=(const aabbRenderer&) = delete;
        public:

            aabbRenderer(const Device* dvc);
			~aabbRenderer();

			void Clear();
            void Init(const VkRenderPass& renderpass, const glm::mat4& projection, const GraphicsPipelineInfo& pipeline_info = GraphicsPipelineInfo());
            void RecordCommands(const VkCommandBuffer& cmd, const VkCommandBufferBeginInfo& begin_info, const glm::mat4& view, const VkViewport& viewport, const VkRect2D& scissor);
            void AddAABB(const AABB& aabb);

        private:

            struct aabb_push_data {
                glm::mat4 view, projection;
            } pushData;

            void createPipelineLayout();
            void createShaders();
            void createBuffers();
            void setupGraphicsPipelineInfo(const GraphicsPipelineInfo& pipeline_info);
            void createGraphicsPipeline(const VkRenderPass& renderpass);

            void rebuildBuffers();
            void updateVBO(const VkCommandBuffer& cmd);
            bool updateRequired = false;
            std::vector<glm::vec3> vertices;
			std::vector<uint16_t> indices;

            const Device* device;

            std::unique_ptr<Buffer> vbo, ebo;
            std::unique_ptr<ShaderModule> vert, frag;
            std::unique_ptr<GraphicsPipeline> pipeline;
            std::unique_ptr<PipelineCache> pipelineCache;
            std::unique_ptr<PipelineLayout> pipelineLayout;

            VkBufferMemoryBarrier updateBarrier;
            GraphicsPipelineInfo pipelineStateInfo;
		    VkGraphicsPipelineCreateInfo pipelineCreateInfo;

        };

    }

}

#endif //!VULPES_VK_AABB_RENDERER_H