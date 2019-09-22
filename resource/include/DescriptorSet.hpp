#pragma once
#ifndef VULPES_VK_DESCRIPTOR_SET_H
#define VULPES_VK_DESCRIPTOR_SET_H
#include "vpr_stdafx.h"
#include <memory>

namespace vpr
{

    /**RAII wrapper around a descriptor set, simplifying adding individual descriptor bindings for whatever stage they're required at. Wrapping descriptor functionality
     * in your own way for projects that are even slightly more advanced than test rigs is probably wise, however. Managing descriptor state, reducing bindings, and 
     * coalescing what features/resources you can is for the best - along with using update templates, which this implementation does not support.
     * \ingroup Resources
    */
    class VPR_API DescriptorSet
    {
        DescriptorSet(const DescriptorSet&) = delete;
        DescriptorSet& operator=(const DescriptorSet&) = delete;
    public:

        DescriptorSet(const VkDevice& parent);
        ~DescriptorSet();

        DescriptorSet(DescriptorSet&& other) noexcept;
        DescriptorSet& operator=(DescriptorSet&& other) noexcept;

        /**Sets the required image info for the descriptor at the given index */
        void AddDescriptorInfo(VkDescriptorImageInfo info, const VkDescriptorType type, const size_t item_binding_idx);
        /**Functionally the same as AddDescriptorInfo with VkDescriptorImageInfo. */
        void AddDescriptorInfo(VkDescriptorBufferInfo info, const VkDescriptorType descr_type, const size_t item_binding_idx);
        /**Add info for a texel buffer*/
        void AddDescriptorInfo(VkDescriptorBufferInfo info, const VkBufferView view, const VkDescriptorType type, const size_t idx);
        /**Binds a separable sampler object to the given index.*/
        void AddSamplerBinding(const size_t idx, const VkSampler sampler_handler);

        /**Call after all descriptor bindings and infos required have been added, 
        *  and make sure you have enough space in the given pool for all of these resources. 
        */
        void Init(const VkDescriptorPool& parent_pool, const VkDescriptorSetLayout& set_layout);
        // Calls vkUpdateDescriptorSets, updating the bindings with potentially new handles representing different buffers
        void Update() const;
        // Clears the write descriptors and info vectors, requiring an update call after re-adding the descriptors
        void Reset();
      
        const VkDescriptorSet& vkHandle() const noexcept;
  
    private:

        void allocate(const VkDescriptorPool& parent_pool, const VkDescriptorSetLayout& set_layout) const;
        void update() const;

        VkDevice device{ VK_NULL_HANDLE };
        mutable VkDescriptorSet handle{ VK_NULL_HANDLE };
        mutable std::unique_ptr<struct DescriptorSetImpl> impl{ nullptr };
    };

}

#endif //!VULPES_VK_DESCRIPTOR_SET_H
