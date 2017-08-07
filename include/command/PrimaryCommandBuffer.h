#ifndef VULPES_VK_PRIMARY_COMMAND_BUFFER_H
#define VULPES_VK_PRIMARY_COMMAND_BUFFER_H

#include "vpr_stdafx.h"
#include "ForwardDecl.h"

namespace vulpes {

    class PrimaryCommandBuffer {
        PrimaryCommandBuffer(const PrimaryCommandBuffer&) = delete;
        PrimaryCommandBuffer& operator=(const PrimaryCommandBuffer&) = delete;
    public:
        
        PrimaryCommandBuffer(CommandPool* parent_pool);

        void Begin(const VkRenderPassBeginInfo& begin_info);
        void End();
        void Reset();

        const VkCommandBuffer& vkHandle() const noexcept;

    private:
        const Device* device;
        CommandPool* parentPool;
        VkCommandBuffer handle;
        size_t parentIdx;
        std::vector<SecondaryCommandBuffer> secondaryBuffers;
    };

}

#endif //!VULPES_VK_PRIMARY_COMMAND_BUFFER_H