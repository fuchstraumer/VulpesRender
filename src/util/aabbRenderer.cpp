#include "..\..\include\util\aabbRenderer.h"
#include "..\..\include\util\aabbRenderer.h"
#include "vpr_stdafx.h"
#include "util/aabbRenderer.h"
#include "core/LogicalDevice.h"


namespace vulpes {

    namespace util {

        aabbRenderer::aabbRenderer(const Device* dvc) : device(dvc), updateBarrier(vk_buffer_memory_barrier_info_base) {}

		aabbRenderer::~aabbRenderer() {
			Clear();
		}

		void aabbRenderer::Clear() {
			vbo.reset();
			ebo.reset();
			vertices.clear();
			vertices.shrink_to_fit();
			indices.clear();
			indices.shrink_to_fit();
			vert.reset();
			frag.reset();
			pipelineLayout.reset();
			pipeline.reset();
			pipelineCache.reset();
		}

		void aabbRenderer::Init(const VkRenderPass& renderpass, const glm::mat4& projection, const GraphicsPipelineInfo& pipeline_info) {

            pushData.projection = projection;

            createShaders();
            createPipelineLayout();

            createBuffers();
            updateBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            updateBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_MEMORY_READ_BIT;
            updateBarrier.buffer = vbo->vkHandle();
            updateBarrier.size = vbo->Size();

            setupGraphicsPipelineInfo(pipeline_info);
            createGraphicsPipeline(renderpass);

        }

        void aabbRenderer::createShaders() {

            vert = std::make_unique<ShaderModule>(device, "./rsrc/shaders/aabb/aabb.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
            frag = std::make_unique<ShaderModule>(device, "./rsrc/shaders/aabb/aabb.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

        }

        void aabbRenderer::createPipelineLayout() {
            
            pipelineLayout = std::make_unique<PipelineLayout>(device);
            pipelineLayout->Create({ VkPushConstantRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushData) } });

        }

        void aabbRenderer::createBuffers() {

            vbo = std::make_unique<Buffer>(device);
			ebo = std::make_unique<Buffer>(device);

            if(!vertices.empty()) {
                rebuildBuffers();
            }

        }

        void aabbRenderer::setupGraphicsPipelineInfo(const GraphicsPipelineInfo& pipeline_info = GraphicsPipelineInfo()) {

			pipelineStateInfo = pipeline_info;

            static const VkVertexInputBindingDescription bind_descr{ 0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX };

            static const std::array<VkVertexInputAttributeDescription, 1> attr_descr{
                VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
            };

			pipelineStateInfo.AssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			pipelineStateInfo.RasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;

            pipelineStateInfo.VertexInfo.vertexBindingDescriptionCount = 1;
            pipelineStateInfo.VertexInfo.pVertexBindingDescriptions = &bind_descr;
            pipelineStateInfo.VertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attr_descr.size());
            pipelineStateInfo.VertexInfo.pVertexAttributeDescriptions = attr_descr.data();

            pipelineStateInfo.DynamicStateInfo.dynamicStateCount = 2;
            static const VkDynamicState states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
            pipelineStateInfo.DynamicStateInfo.pDynamicStates = states;

            pipelineStateInfo.DepthStencilInfo.depthTestEnable = VK_TRUE;
            pipelineStateInfo.DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

            pipelineStateInfo.RasterizationInfo.cullMode = VK_CULL_MODE_NONE;

        }

        void aabbRenderer::createGraphicsPipeline(const VkRenderPass& renderpass) {

            pipelineCreateInfo = pipelineStateInfo.GetPipelineCreateInfo();
            pipelineCreateInfo.renderPass = renderpass;
            pipelineCreateInfo.layout = pipelineLayout->vkHandle();
			pipelineCreateInfo.subpass = 0;

			std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{ vert->PipelineInfo(), frag->PipelineInfo() };
			pipelineCreateInfo.stageCount = static_cast<uint32_t>(shader_stages.size());
			pipelineCreateInfo.pStages = shader_stages.data();

            pipeline = std::make_unique<GraphicsPipeline>(device);
            pipelineCache = std::make_unique<PipelineCache>(device, static_cast<uint16_t>(typeid(aabbRenderer).hash_code()));
            pipeline->Init(pipelineCreateInfo, pipelineCache->vkHandle());

        }

        void aabbRenderer::RecordCommands(const VkCommandBuffer& cmd, const VkCommandBufferBeginInfo& begin_info, const glm::mat4& view, 
            const VkViewport& viewport, const VkRect2D& scissor) {

            if(vertices.empty()) {
                vkBeginCommandBuffer(cmd, &begin_info);
                vkEndCommandBuffer(cmd);
                return;
            }

            pushData.view = view;

            VkResult result = vkBeginCommandBuffer(cmd, &begin_info);
            VkAssert(result);

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkHandle());
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);
            vkCmdPushConstants(cmd, pipelineLayout->vkHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushData), &pushData);
			VkDeviceSize offsets[1]{ 0 };
			vkCmdBindVertexBuffers(cmd, 0, 1, &vbo->vkHandle(), offsets);
			vkCmdBindIndexBuffer(cmd, ebo->vkHandle(), 0, VK_INDEX_TYPE_UINT16);
			vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

            result = vkEndCommandBuffer(cmd);
            VkAssert(result);

        }

        void aabbRenderer::rebuildBuffers() {

            vbo->Destroy();
            vbo->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, sizeof(glm::vec3) * vertices.size());
            vbo->CopyToMapped(vertices.data(), vbo->Size(), 0);

			ebo->Destroy();
			ebo->CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, sizeof(uint16_t) * indices.size());
			ebo->CopyToMapped(indices.data(), ebo->Size(), 0);

        }

        void aabbRenderer::AddAABB(const AABB& aabb) {

            vertices.reserve(vertices.capacity() + 8);
           
            auto new_vertices = {
                aabb.Min,
                glm::vec3(aabb.Max.x, aabb.Min.y, aabb.Min.z),
                glm::vec3(aabb.Min.x, aabb.Min.y, aabb.Max.z),
                glm::vec3(aabb.Max.x, aabb.Min.y, aabb.Max.z),
                aabb.Max,
                glm::vec3(aabb.Min.x, aabb.Max.y, aabb.Min.z),
                glm::vec3(aabb.Max.x, aabb.Max.y, aabb.Min.z), 
                glm::vec3(aabb.Min.x, aabb.Max.y, aabb.Max.z)
            };

			std::initializer_list<uint16_t> new_indices = {
				0, 1, 1, 3, 3, 2, 2, 0,
				0, 5, 1, 6, 2, 7, 3, 4, 
				5, 6, 6, 4, 4, 7, 7, 5,
			};

            vertices.insert(vertices.end(), new_vertices);
			indices.insert(indices.end(), new_indices);
			
            rebuildBuffers();

        }

    }

}