#include "vpr_stdafx.h"
#include "render/PickingPass.hpp"
#include "objects/vertex_t.hpp"
#include "BaseScene.hpp"

namespace vulpes {

    bool PickingPass::blitAttachment = false;


    PickingPass::PickingPass(const Device* _device, const Swapchain* _swapchain, const glm::mat4& projection) : device(_device), swapchain(_swapchain) {
        uboData.projection = projection;
        createGraphicsCmdPools();
        createPipelineLayout();
        createPipelineCache();
        createShaders();
        createOffscreenFramebuffers();
        createOutputAttachment();
        setPipelineStateInfo();
        createGraphicsPipeline();
    }

    PickingPass::~PickingPass() {
        primaryPool.reset();
        secondaryPool.reset();
        vert.reset();
        frag.reset();
        pipelineCache.reset();
        pipelineLayout.reset();
        destFramebuffers.reset();
        destImage.reset();
    }

    std::future<void> PickingPass::readbackAttachment(const size_t& curr_frame_idx) {
        return std::async(std::launch::async, updateCpuAttachment, device, swapchain, destFramebuffers->GetAttachment(curr_frame_idx).vkHandle(), destImage->vkHandle(), primaryPool.get());
    }

    void PickingPass::updateCpuAttachment(const Device* dvc, const Swapchain* swapchain, const VkImage& color_attachment, const VkImage& dest_image, CommandPool* command_pool) {
        
        static VkImageMemoryBarrier from_color_attachment_layout_barrier {
            VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            nullptr,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            VK_NULL_HANDLE,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        };

        from_color_attachment_layout_barrier.image = color_attachment;
        
        static VkImageMemoryBarrier to_transfer_layout_barrier {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            nullptr,
            VK_ACCESS_MEMORY_READ_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            VK_NULL_HANDLE,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        };

        to_transfer_layout_barrier.image = dest_image;

        static VkImageMemoryBarrier to_color_attachment_layout_barrier {
            VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            nullptr,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            VK_NULL_HANDLE,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        };

        to_color_attachment_layout_barrier.image = color_attachment;

        static VkImageMemoryBarrier to_host_layout_barrier {
            VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            nullptr,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_MEMORY_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_QUEUE_FAMILY_IGNORED,
            VK_QUEUE_FAMILY_IGNORED,
            VK_NULL_HANDLE,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        };

        to_host_layout_barrier.image = dest_image;

        static VkCommandBufferBeginInfo begin_info = vk_command_buffer_begin_info_base;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

        VkCommandBuffer cmd_buffer = command_pool->GetCmdBuffer(0);

        vkBeginCommandBuffer(cmd_buffer, &begin_info);
            vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &from_color_attachment_layout_barrier);
            vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_transfer_layout_barrier);
            if(blitAttachment) {
                VkOffset3D blit_offset{ static_cast<int32_t>(swapchain->Extent.width), static_cast<int32_t>(swapchain->Extent.height), int32_t(1) };
                VkImageBlit blit_region;
                blit_region.srcSubresource = VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
                blit_region.dstOffsets[0] = blit_offset,
                blit_region.dstSubresource = VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
                blit_region.dstOffsets[0] = blit_offset;
                vkCmdBlitImage(cmd_buffer, color_attachment, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dest_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit_region, VK_FILTER_LINEAR);
            }
            else {
                static VkImageCopy image_copy{ 
                    VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
                    VkOffset3D{ 0, 0, 0 },
                    VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
                    VkOffset3D{ 0, 0, 0 },
                    VkExtent3D{ swapchain->Extent.width, swapchain->Extent.height, 1 }
                };
                vkCmdCopyImage(cmd_buffer, color_attachment, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dest_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);
            }
            vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_color_attachment_layout_barrier);
            vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 0, nullptr, 1, &to_host_layout_barrier);
        vkEndCommandBuffer(cmd_buffer);

        VkFence fence;
        vkCreateFence(dvc->vkHandle(), &vk_fence_create_info_base, nullptr, &fence);

        VkSubmitInfo submission_info = vk_submit_info_base;
        submission_info.commandBufferCount = 1;
        submission_info.pCommandBuffers = &cmd_buffer;
        vkQueueSubmit(dvc->GraphicsQueue(), 1, &submission_info, fence);
        vkWaitForFences(dvc->vkHandle(), 1, &fence, VK_TRUE, vk_default_fence_timeout);
        vkResetFences(dvc->vkHandle(), 1, &fence);

        command_pool->ResetCmdBuffer(0);
        return;
    }

    void PickingPass::RenderPickingPass(const size_t & frame_idx, const VkViewport & viewport, const VkRect2D & scissor, const glm::mat4 & view) {
        uboData.view = view;

        static VkCommandBufferBeginInfo primary_cmd_buffer_begin_info{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            nullptr,
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            nullptr
        };

        static VkCommandBufferInheritanceInfo secondary_cmd_buffer_inheritance_info = vulpes::vk_command_buffer_inheritance_info_base;
        secondary_cmd_buffer_inheritance_info.renderPass = destFramebuffers->GetRenderpass();
        secondary_cmd_buffer_inheritance_info.framebuffer = destFramebuffers->GetFramebuffer(frame_idx);
        secondary_cmd_buffer_inheritance_info.subpass = 0;

        static VkCommandBufferBeginInfo secondary_cmd_buffer_begin_info{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            nullptr,
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
            &secondary_cmd_buffer_inheritance_info
        };

        using map_entry = decltype(pickingObjects)::value_type;
        auto render_object = [&](const VkCommandBuffer& cmd, VkCommandBufferBeginInfo begin_info, map_entry& _object) {
            vkBeginCommandBuffer(cmd, &begin_info);
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline->vkHandle());
                vkCmdSetViewport(cmd, 0, 1, &viewport);
                vkCmdSetScissor(cmd, 0, 1, &scissor);
                TriangleMesh* mesh = (TriangleMesh*)_object.second;
                uboData.model = mesh->GetModelMatrix();
                vkCmdPushConstants(cmd, pipelineLayout->vkHandle(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) * 3, &uboData);
                vkCmdPushConstants(cmd, pipelineLayout->vkHandle(), VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4) * 3, sizeof(uint32_t), &_object.first);
                mesh->Render(cmd);
           vkEndCommandBuffer(cmd);
        };

        size_t num_objects = pickingObjects.size();
        if(num_objects != secondaryPool->size()) {
            secondaryPool->FreeCommandBuffers();
            secondaryPool->AllocateCmdBuffers(num_objects, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
        }

        std::vector<VkCommandBuffer> secondary_buffers;

        vkBeginCommandBuffer(primaryPool->GetCmdBuffer(frame_idx), &primary_cmd_buffer_begin_info);
            vkCmdBeginRenderPass(primaryPool->GetCmdBuffer(frame_idx), &destFramebuffers->GetRenderpassBeginInfo(), VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
            size_t curr_idx = 0;          
            for(auto& obj : pickingObjects) {
                render_object(secondaryPool->GetCmdBuffer(curr_idx), secondary_cmd_buffer_begin_info, obj);
                secondary_buffers.push_back(secondaryPool->GetCmdBuffer(curr_idx));
                ++curr_idx;
            }
            vkCmdExecuteCommands(primaryPool->GetCmdBuffer(frame_idx), static_cast<uint32_t>(secondary_buffers.size()), secondary_buffers.data());
            vkCmdEndRenderPass(primaryPool->GetCmdBuffer(frame_idx));
        vkEndCommandBuffer(primaryPool->GetCmdBuffer(frame_idx));
    }

    void PickingPass::createPipelineCache() {
        pipelineCache = std::make_unique<PipelineCache>(device, static_cast<uint16_t>(typeid(PickingPass).hash_code()));
    }

    void PickingPass::setPipelineStateInfo() {

        constexpr static VkDynamicState dynamic_states[2]{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        pipelineStateInfo.DynamicStateInfo.dynamicStateCount = 2;
        pipelineStateInfo.DynamicStateInfo.pDynamicStates = dynamic_states;

        pipelineStateInfo.MultisampleInfo.sampleShadingEnable = BaseScene::SceneConfiguration.EnableMSAA;
        if (pipelineStateInfo.MultisampleInfo.sampleShadingEnable) {
            pipelineStateInfo.MultisampleInfo.rasterizationSamples = BaseScene::SceneConfiguration.MSAA_SampleCount;
        }

        pipelineStateInfo.VertexInfo.vertexBindingDescriptionCount = 1;
        pipelineStateInfo.VertexInfo.pVertexBindingDescriptions = &vertex_t::bindingDescription;
        pipelineStateInfo.VertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_t::attributeDescriptions.size());
        pipelineStateInfo.VertexInfo.pVertexAttributeDescriptions = vertex_t::attributeDescriptions.data();

        pipelineStateInfo.ColorBlendInfo.attachmentCount = 2;
        constexpr static VkPipelineColorBlendAttachmentState color_states[2]{ 
            vk_pipeline_color_blend_attachment_info_base,
            VkPipelineColorBlendAttachmentState{ VK_FALSE, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ONE,
                                                 VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, }
        };
        pipelineStateInfo.ColorBlendInfo.pAttachments = color_states;

    }

    void PickingPass::createGraphicsPipeline() {

        pipelineCreateInfo = pipelineStateInfo.GetPipelineCreateInfo();
        const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{
            vert->PipelineInfo(), frag->PipelineInfo()
        };

        pipelineCreateInfo.stageCount = static_cast<uint32_t>(shader_stages.size());
        pipelineCreateInfo.pStages = shader_stages.data();

        pipelineCreateInfo.layout = pipelineLayout->vkHandle();
        pipelineCreateInfo.renderPass = destFramebuffers->GetRenderpass();
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineCreateInfo.basePipelineIndex = -1;

        graphicsPipeline = std::make_unique<GraphicsPipeline>(device);
        graphicsPipeline->Init(pipelineCreateInfo, pipelineCache->vkHandle());

    }

    void PickingPass::createGraphicsCmdPools() {
        
        VkCommandPoolCreateInfo pool_info = vk_command_pool_info_base;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = device->QueueFamilyIndices.Graphics;
		primaryPool = std::make_unique<CommandPool>(device, pool_info, true);

		VkCommandBufferAllocateInfo alloc_info = vk_command_buffer_allocate_info_base;
		primaryPool->AllocateCmdBuffers(swapchain->ImageCount, alloc_info);

		secondaryPool = std::make_unique<CommandPool>(device, pool_info, false);
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        secondaryPool->AllocateCmdBuffers(swapchain->ImageCount, alloc_info);

    }

    void PickingPass::createPipelineLayout() {
        
        constexpr static VkPushConstantRange vert_range{
            VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) * 3
        };

        constexpr static VkPushConstantRange frag_range{
            VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4) * 3, sizeof(uint32_t)
        };

        pipelineLayout = std::make_unique<PipelineLayout>(device);
        pipelineLayout->Create({ vert_range, frag_range });

    }

    void PickingPass::createShaders() {
        vert = std::make_unique<ShaderModule>(device, BaseScene::SceneConfiguration.ResourcePathPrefixStr + "rsrc/shaders/picking/picking.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
        frag = std::make_unique<ShaderModule>(device, BaseScene::SceneConfiguration.ResourcePathPrefixStr + "rsrc/shaders/picking/picking.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    void PickingPass::createOffscreenFramebuffers() {
        destFramebuffers = std::make_unique<OffscreenFramebuffers<picking_framebuffer_t>>(device, swapchain);
        destFramebuffers->Create();
    }

    void PickingPass::createOutputAttachment() {
        destImage = std::make_unique<Image>(device);
        VkImageCreateInfo output_image_info = vk_image_create_info_base;
        output_image_info.format = VK_FORMAT_R32_UINT;
        output_image_info.extent = VkExtent3D{ swapchain->Extent.width, swapchain->Extent.height, 1 };
        output_image_info.tiling = VK_IMAGE_TILING_LINEAR;
        output_image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        output_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; 
        output_image_info.mipLevels = 1;
        output_image_info.arrayLayers = 1;
        destImage->Create(output_image_info, VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT));
    }

}