#include "vpr_stdafx.h"
#include "util/TransferTaskPool.hpp"
#include "core/LogicalDevice.hpp"
#include "resource/Allocator.hpp"
#include "resource/Buffer.hpp"
namespace vulpes {

    namespace util {

        void TransferTaskPool::AddTransferTask(const void* src_data, const VkDeviceSize& copy_size, const VkBuffer& destination_buffer) {

            

            static auto copy_task = [&](const void* _src_data, const VkDeviceSize& _copy_size) {

                Allocation alloc;
                VkBuffer staging_buffer;
                Buffer::CreateStagingBuffer(device, copy_size, staging_buffer, alloc);
                void* mapped;
                alloc.Map(copy_size, 0, mapped);
                memcpy(mapped, _src_data, copy_size);
                alloc.Unmap();

                const VkBufferCopy buffer_copy{ 0, 0, copy_size };

            };

            

            
        }

    }

}