#include "vpr_stdafx.h"
#include "util/aabbRenderer.h"
#include "core/LogicalDevice.h"


namespace vulpes {

    namespace util {

        aabbRenderer::aabbRenderer(const Device* dvc) : device(dvc), updateBarrier(vk_buffer_memory_barrier_info_base) {}

        void aabbRenderer::Init(const VkRenderPass& renderpass, const glm::mat4& projection) {

            pushData.projection = projection;

            createShaders();
            createPipelineLayout();

            createBuffers();
            updateBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            updateBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_MEMORY_READ_BIT;
            updateBarrier.buffer = vbo->vkHandle();
            updateBarrier.size = vbo->Size();

            setupGraphicsPipelineInfo();
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
            staging = std::make_unique<Buffer>(device);

            if(!vertices.empty()) {
                rebuildBuffers();
            }

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

            if(updateRequired) {
                updateVBO(cmd);
            }

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkHandle());
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);
            vkCmdPushConstants(cmd, pipelineLayout->vkHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushData), &pushData);
            vkCmdDraw(cmd, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

            result = vkEndCommandBuffer(cmd);
            VkAssert(result);

        }

        void aabbRenderer::updateVBO(const VkCommandBuffer& cmd) {

            VkBufferCopy staging_to_device{ 0, 0, sizeof(glm::vec3) * vertices.size() };
            // update, then specify barrier to say no rendering until next frame reached (since transfer comes after shader, will "wrap around")
            vkCmdCopyBuffer(cmd, staging->vkHandle(), vbo->vkHandle(), 1, &staging_to_device);
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1, &updateBarrier, 0, nullptr);

        }

        void aabbRenderer::rebuildBuffers() {

            // make sure to recreate vbo to account for updated size.
            vbo->Destroy();
            vbo->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sizeof(glm::vec3) * vertices.size());

            staging->Destroy();
            staging->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, sizeof(glm::vec3) * vertices.size());
            staging->CopyToMapped(vertices.data(), staging->Size(), 0);

            updateRequired = true;

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

            vertices.insert(vertices.end(), new_vertices);
            rebuildBuffers();

        }

    }

}