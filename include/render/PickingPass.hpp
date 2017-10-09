#ifndef PICKING_OBJECT_HPP
#define PICKING_OBJECT_HPP

#include "vpr_stdafx.h"
#include "objects/TriangleMesh.hpp"
#include "resource/Image.hpp"
#include "render/OffscreenFramebuffers.hpp"
#include "command/CommandPool.hpp"
#include "resource/PipelineLayout.hpp"
#include "render/GraphicsPipeline.hpp"
#include "resource/PipelineCache.hpp"
#include "resource/ShaderModule.hpp"
#include <unordered_map>

namespace vulpes {

    template<typename T>
    using is_triangle_mesh = typename std::enable_if<std::is_base_of<TriangleMesh, T>::value>::type;

    class PickingPass {
    public:

        struct ubo_data_t {
            glm::mat4 model;
            glm::mat4 view;
            glm::mat4 projection;
            uint16_t objectID;
        } uboData;

        PickingPass(const Device* _device, const Swapchain* _swapchain, const glm::mat4& projection);
        ~PickingPass();

        uint32_t AddObjectForPicking(TriangleMesh* triangle_mesh) {
           
            uint32_t object_idx = static_cast<uint32_t>(pickingObjects.size());
            auto inserted = pickingObjects.insert(std::make_pair(triangle_mesh, object_idx));

        }

        bool WasObjectPicked(const size_t& mouse_x, const size_t& mouse_y, const uint32_t& uuid) {
            readbackFuture.get();
            return false;
        }

        void RenderPickingPass(const size_t& frame_idx, const VkViewport& viewport, const VkRect2D& scissor, const glm::mat4& view);

    private:

        const Device* device;
        const Swapchain* swapchain;

        void createPipelineCache();
        void setPipelineStateInfo();
        void createGraphicsPipeline();
        void createGraphicsCmdPools();
        void createPipelineLayout();
        void createShaders();
        void createOffscreenFramebuffers();
        void createOutputAttachment();

        std::future<void> readbackAttachment(const size_t& curr_frame_idx);
        static void updateCpuAttachment(const Device* dvc, const Swapchain* swapchain, const VkImage& color_attachment, const VkImage& dest_image, CommandPool* command_pool);
        static bool blitAttachment;

        // All objects use these.
        std::unique_ptr<PipelineCache> pipelineCache;
        std::unique_ptr<GraphicsPipeline> graphicsPipeline;
        std::unique_ptr<PipelineLayout> pipelineLayout;
        std::unique_ptr<ShaderModule> vert, frag;
        std::unordered_map<TriangleMesh*, uint32_t> pickingObjects;
        std::unique_ptr<CommandPool> primaryPool, secondaryPool;
        std::unique_ptr<OffscreenFramebuffers<picking_framebuffer_t>> destFramebuffers;
        std::unique_ptr<Image> destImage;
        std::future<void> readbackFuture;
        GraphicsPipelineInfo pipelineStateInfo;
        VkGraphicsPipelineCreateInfo pipelineCreateInfo;
    };

}

#endif //!PICKING_OBJECT_HPP