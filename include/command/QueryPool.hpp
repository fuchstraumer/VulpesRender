#pragma once
#ifndef VULPESRENDER_QUERY_POOL_HPP
#define VULPESRENDER_QUERY_POOL_HPP

#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include "core/LogicalDevice.hpp"

namespace vpr {

    using occlusion_query_t = std::integral_constant<VkQueryType, VK_QUERY_TYPE_OCCLUSION>;
    using pipeline_stats_query_t = std::integral_constant<VkQueryType, VK_QUERY_TYPE_PIPELINE_STATISTICS>;
    using exec_time_query_t = std::integral_constant<VkQueryType, VK_QUERY_TYPE_TIMESTAMP>;
    // This is an occlusion query done with high precision: instead of returning obscurance booleans,
    // it checks the visible area per object.
    using area_query_t = std::integral_constant<VkQueryType, VK_QUERY_TYPE_RANGE_SIZE>;

    namespace detail {
        constexpr size_t get_stat_query_count(const uint32_t& i) {
            uint32_t c = i - ((i >> 1) & 0x55555555);
            c = (i & 0x33333333) + ((i >> 2) & 0x33333333);
            return (((c + (c >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
        }
    }

    template<typename query_type, size_t num_queries>
    class QueryPool { 
        QueryPool(const QueryPool&) = delete;
        QueryPool& operator=(const QueryPool&) = delete;
    public:

        QueryPool(const Device* device);
        template<VkQueryPipelineStatisticFlagBits stats_queries>
        QueryPool(const Device* device);
        ~QueryPool();

        const VkQueryPool& vkHandle() const noexcept;

        void Reset(const VkCommandBuffer& cmd, const size_t& query_idx);
        void ResetAll(const VkCommandBuffer& cmd);

        void Begin(const VkCommandBuffer& cmd, const size_t& idx);
        void BeginAll(const VkCommandBuffer& cmd);

        void End(const VkCommandBuffer& cmd, const size_t& idx);
        void EndAll(const VkCommandBuffer& cmd, const size_t& idx);

        VkResult GetResults();
        void WaitForResults();

        const std::array<uint32_t, num_queries>& GetData() const noexcept;

    private:

        const Device* device;
        VkQueryPool handle;
        VkQueryPoolCreateInfo createInfo;
        std::array<bool, num_queries> activeQueries;
        std::array<uint32_t, num_queries> queryResults;

    };

    template<typename query_type, size_t num_queries>
    inline QueryPool<query_type, num_queries>::QueryPool(const Device* _device) : device(_device), createInfo(vk_query_pool_create_info_base) {
        static_assert(query_type != pipeline_stats_query_t, "pipeline_stats_query_t QueryPool's require an additional template parameter,
            specifying exactly which pipeline statistics will be queried for.");
        createInfo.queryType = query_type;
        createInfo.queryCount = num_queries;

        VkResult result = vkCreateQueryPool(device->vkHandle(), &createInfo, nullptr, &handle);
        VkAssert(result);
        
    }

    template<size_t num_queries>
    template<VkQueryPipelineStatisticFlagBits stats_queries>
    inline QueryPool<pipeline_stats_query_t,num_queries + detail::get_stat_query_count(stats_queries)>::QueryPool(const Device* _device) : 
        createInfo(vk_query_pool_create_info_base), device(_device) {
        
        createInfo.pipelineStatistics = VkQueryPipelineStatisticFlags(flag_bits);
        createInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
        createInfo.queryCount = num_queries;

        VkResult result = vkCreateQueryPool(device->vkHandle(), &createInfo, nullptr, &handle);
        VkAssert(result);
    }
    
    template<typename query_type, size_t num_queries>
    inline QueryPool<query_type, num_queries>::~QueryPool() {
        vkDestroyQueryPool(device->vkHandle(), handle, nullptr);
    }

    template<typename query_type, size_t num_queries>
    inline void QueryPool<query_type, num_queries>::ResetAll(const VkCommandBuffer& cmd) {

        // want all values to be false.
        bool can_reset = std::all_of(activeQueries.cbegin(), activeQueries.cend(), [](const bool& val) { return !val; });
        if(!can_reset) {
            std::cerr << "Can't reset command pool with active queries!\n"; 
            throw std::runtime_error("Tried to reset command pool with active queries.");
        }

        vkCmdResetQueryPool(cmd, handle, 0, num_queries);

    }

    template<typename query_type, size_t num_queries>
    inline void QueryPool<query_type, num_queries>::Reset(const VkCommandBuffer& cmd, const size_t& idx) {
        assert(idx < num_queries);
        vkCmdResetQueryPool(cmd, handle, idx, 1);
    }

    template<typename query_type, size_t num_queries>
    inline void QueryPool<query_type, num_queries>::Begin(const VkCommandBuffer& cmd, const size_t& idx) {
        
        if(activeQueries[idx]) {
            throw std::runtime_error("Tried to begin query that is already active!");
        } 
        else {
            activeQueries[idx] = true;
        }

        vkCmdBeginQuery(cmd, handle, idx, 0);
    }

    template<size_t num_queries>
    inline void QueryPool<area_query_t, num_queries>::Begin(const VkCommandBuffer& cmd, const size_t& idx) {
        
        if(activeQueries[idx]) {
            throw std::runtime_error("Tried to begin query that is already active!");
        } 
        else {
            activeQueries[idx] = true;
        }

        vkCmdBeginQuery(cmd, handle, idx, VK_QUERY_CONTROL_PRECISE_BIT);
    }

    template<typename query_type, size_t num_queries>
    inline void QueryPool<query_type, num_queries>::BeginAll(const VkCommandBuffer& cmd) {
        for(size_t i = 0; i < num_queries; ++i) {
            Begin(cmd, i);
        }
    }

    template<typename query_type, size_t num_queries>
    inline void QueryPool<query_type, num_queries>::End(const VkCommandBuffer& cmd, const size_t& idx) {
        vkCmdEndQuery(cmd, handle, idx);
        activeQueries[idx] = false;
    }

    template<typename query_type, size_t num_queries>
    inline void QueryPool<query_type, num_queries>::EndAll(const vkCommandBuffer& cmd, const size_t& idx) {
        for(size_t i = 0; i < num_queries; ++i) {
            End(cmd, i);
        }
    }

    template<typename query_type, size_t num_queries>
    inline VkResult QueryPool<query_type, num_queries>::GetResults() {
        return vkGetQueryPoolResults(device->vkHandle(), handle, 0, num_queries, sizeof(uint32_t) * num_queries,
            queryResults.data(), sizeof(uint32_t), 0);
    }

    template<typename query_type, size_t num_queries>
    inline void QueryPool<query_type, num_queries>::WaitForResults() {
        VkResult result vkGetQueryPoolResults(device->vkHandle(), handle, 0, num_queries, sizeof(uint32_t) * num_queries,
            queryResults.data(), sizeof(uint32_t), VK_QUERY_RESULT_WAIT_BIT);
        // We don't assert on the other Getter function because it can return something besides VK_SUCCESS and still be considered
        // fine. Here, we better get VK_SUCCESS since we are supposed to be waiting for valid data.
        VkAssert(result);
    }
}

#endif //!VULPESRENDER_QUERY_POOL_HPP