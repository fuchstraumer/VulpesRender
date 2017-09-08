#ifndef VULPES_VK_THREADED_TRANSFER_TASK_POOL_HPP
#define VULPES_VK_THREADED_TRANSFER_TASK_POOL_HPP

#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include "TaskPool.hpp"
#include "command/TransferPool.hpp"
namespace vulpes {

    namespace util {

        class TransferTaskPool {
            TransferTaskPool(const TransferTaskPool&) = delete;
            TransferTaskPool& operator=(const TransferTaskPool&) = delete;
        public:

            TransferTaskPool(const Device* _device);
            
            void AddTransferTask(const void* src_data, const VkDeviceSize& copy_size, const VkBuffer& destination_buffer);

        private:
            
            const Device* device;
            std::unique_ptr<TaskPool> taskPool;
            std::unique_ptr<TransferPool> transferCmdPool;
        };

    }

}

#endif //!VULPES_VK_THREADED_TRANSFER_TASK_POOL_HPP