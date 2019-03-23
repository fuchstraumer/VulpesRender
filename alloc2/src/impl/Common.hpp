#pragma once
#ifndef VPR_ALLOC2_COMMON_FUNCTIONALITY_HPP
#define VPR_ALLOC2_COMMON_FUNCTIONALITY_HPP
#include <vulkan/vulkan.h>
#include <utility>
#include <vector>
#include <list>
#include <limits>

namespace vpr {

#if defined(__APPLE__) || defined(__ANDROID__) 
    void* aligned_alloc_impl(size_t sz, size_t align) {
        if (alignment < sizeof(void*)) {
            alignment = sizeof(void*);
        }

        void* ptr{ nullptr };
        if (posix_memalign(&pointer, align, sz) == 0) {
            return ptr;
        }
        else {
            return nullptr;
        }

    }
#endif

#if defined(_WIN32)
    void* vpr_aligned_alloc(size_t sz, size_t align) {
        return _aligned_malloc(sz, align);
    }
    void vpr_aligned_free(void* ptr) {
        _aligned_free(ptr);
    }
#else
    void* vpr_aligned_alloc(size_t sz, size_t align) {
        return aligned_alloc_impl(align, sz);
    }
    void vpr_aligned_free(void* ptr) {
        free(ptr);
    }
#endif

    enum SuballocationType {
        SuballocTypeFree,
        SuballocTypeUnknown,
        SuballocTypeBuffer,
        SuballocTypeImageUnknown,
        SuballocTypeImageLinear,
        SuballocTypeImageOptimal
    };

    static inline bool CheckBufferImageGranularityConflict(VprSuballocationType type0, VprSuballocationType type1) noexcept {
        
        if (type0 > type1) {
            std::swap(type0, type1);
        }

        switch (type0) {
        case SuballocTypeFree:
            return false;
        case SuballocTypeUnknown:
            return true;
        case SuballocTypeBuffer:
            return (type1 == SuballocTypeImageUnknown) || (type1 == SuballocTypeImageOptimal);
        case SuballocTypeImageUnknown:
            return (type1 == SuballocTypeImageUnknown) || (type1 == SuballocTypeImageLinear) ||
                (type1 == SuballocTypeImageOptimal);
        case SuballocTypeImageLinear:
            return (type1 == SuballocTypeImageOptimal);
        case SuballocTypeImageOptimal:
            return false;
        default:
            return true;
        }

    }

    template<typename CmpLess, typename IteratorType, typename KeyType>
    static IteratorType BinaryFindFirstNotLess(IteratorType begin, IteratorType end, const KeyType& key, CmpLess cmp) {
        size_t down{ 0u };
        size_t up{ end - begin };

        while (down < up) {
            const size_t middle{ (down + up) / 2u };
            if (cmp(*(begin + middle), key)) {
                down = middle + 1;
            }
            else {
                up = middle;
            }
        }

        return begin + down;
    }

    static void* VprMalloc(const VkAllocationCallbacks* alloc_callbacks, size_t size, size_t alignment) {
        if ((alloc_callbacks != nullptr) && (alloc_callbacks->pfnAllocation != nullptr)) {
            return (*alloc_callbacks->pfnAllocation)(alloc_callbacks->pUserData, size, alignment, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
        }
        else {
            return vpr_aligned_alloc(size, alignment);
        }
    }

    static void VprFree(const VkAllocationCallbacks* alloc_callbacks, void* ptr) {
        if ((alloc_callbacks != nullptr) && (alloc_callbacks->pfnFree != nullptr)) {
            (*alloc_callbacks->pfnFree)(alloc_callbacks->pUserData, ptr);
        }
        else {
            return vpr_aligned_free(ptr);
        }
    }

    template<typename T>
    static T* VprAllocate(const VkAllocationCallbacks* alloc_callbacks) {
        return static_cast<T*>(VprMalloc(alloc_callbacks, sizeof(T), alignof(T)));
    }

    template<typename T>
    static T* VprAllocateArray(const VkAllocationCallbacks* alloc_callbacks, size_t count) {
        return static_cast<T*>(VprMalloc(alloc_callbacks, sizeof(T) * count, alignof(T)));
    }

    #define vpr_new(allocator, type) new(VprAllocate<type>(allocator))(type)
    #define vpr_new_array(allocator, type, count) new(VprAllocateArray<type>((allocator), (count)))(type)

    template<typename T>
    static void vpr_delete(const VkAllocationCallbacks* alloc_callbacks, T* ptr) {
        ptr->~T();
        VprFree(alloc_callbacks, ptr);
    }

    template<typename T>
    static void vpr_delete_array(const VkAllocationCallbacks* alloc_callbacks, T* ptr, size_t count) {
        if (ptr != nullptr) {
            for (size_t i = count; --i; ) {
                ptr[i].~T();
            }
            VprFree(alloc_callbacks, ptr);
        }
    }

    template<typename T>
    class VprStlAllocator {
    public:

        const VkAllocationCallbacks* const AllocCallbacks;
        using value_type = T;

        VprStlAllocator(const VkAllocationCallbacks* alloc_callbacks) : AllocCallbacks{ alloc_callbacks } {}
        template<typename U>
        VprStlAllocator(const VprStlAllocator<U>& src) noexcept : AllocCallbacks{ src.AllocCallbacks } {}
        VprStlAllocator& operator=(const VprStlAllocator&) = delete;

        T* allocate(size_t n) {
            return VprAllocateArray<T>(AllocCallbacks, n);
        }

        void deallocate(T* p, size_t n) {
            return VprFree(AllocCallbacks, p);
        }

        template<typename U>
        bool operator==(const VprStlAllocator<U>& rhs) const noexcept {
            return AllocCallbacks == rhs.AllocCallbacks;
        }

        template<typename U>
        bool operator!=(const VprStlAllocator<U>& rhs) const noexcept {
            return AllocCallbacks != rhs.AllocCallbacks;
        }

    };

    template<typename T, typename AllocType>
    using VprVector = std::vector<T, AllocType<T>>;

    template<typename T>
    class VprPoolAllocator {

        VprPoolAllocator(const VprPoolAllocator&) = delete;
        VprPoolAllocator& operator=(const VprPoolAllocator&) = delete;

        union PoolItem {
            uint32_t nextFreeIdx{ std::numeric_limits<uint32_t>::max() };
            T value;
        };

        struct PoolItemBlock {
            PoolItem* Items{ nullptr };
            uint32_t Capacity{ 0u };
            uint32_t FirstFreeIdx{ std::numeric_limits<uint32_t>::max() };
        };

        const VkAllocationCallbacks allocCallbacks{ nullptr };
        const uint32_t firstBlockCapacity{ 0u };
        VprVector<PoolItemBlock, VprStlAllocator<PoolItemBlock>> itemBlocks;

        PoolItemBlock& createNewBlock() {
            const uint32_t new_block_capacity = itemBlocks.empty() ? firstBlockCapacity : itemBlocks.back().Capacity * 3u / 2u;
            itemBlocks.emplace_back(std::move(PoolItemBlock{ vpr_new_array(allocCallbacks, PoolItem, new_block_capacity), new_block_capacity, 0u }));
            PoolItemBlock& result = itemBlocks.back();

            for (uint32_t i = 0; i < new_block_capacity - 1; ++i) {
                result.Items[i].NextFreeIdx = i + 1;
            }

            // not needed: already initialized to this
            //result.Items[new_block_capacity - 1].NextFreeIdx = std::numeric_limits<uint32_t>::max();

            return result;
        }
        
    public:

        VprPoolAllocator(const VkAllocationCallbacks* alloc_callbacks, uint32_t first_block_capacity) : allocCallbacks{ alloc_callbacks }, firstBlockCapacity{ first_block_capacity },
            itemBlocks(VprStlAllocator<PoolItemBlock>(alloc_callbacks)) {}

        ~VprPoolAllocator() {
            clear();
        }

        void clear() {
            for (size_t i = itemBlocks.size(); --i; ) {
                vpr_delete_array(allocCallbacks, itemBlocks[i].Items, itemBlocks[i].Capacity);
            }
            itemBlocks.clear();
            itemBlocks.shrink_to_fit();
        }

        T* allocate() {
            for (size_t i = itemBlocks.size(); --i; ) {
                PoolItemBlock& block = itemBlocks[i];
                if (block.FirstFreeIdx != std::numeric_limits<uint32_t>::max()) {
                    PoolItem* const item = &block.Items[block.FirstFreeIdx];
                    block.FirstFreeIdx = item->nextFreeIdx;
                    return &item->value;
                }
            }

            PoolItemBlock& new_block = createNewBlock();
            PoolItem* const item = &new_block.Items[0u];
            new_block.FirstFreeIdx = item->nextFreeIdx;
            return &item->value;
        }

        void free(T* ptr) {
            for (size_t i = itemBlocks.size(); --i; ) {
                PoolItemBlock& block = itemBlocks[i];
                
                PoolItem* item_ptr{ nullptr };
                memcpy(&item_ptr, &ptr, sizeof(PoolItem*));

                if ((item_ptr >= block.Items) && (item_ptr < block.Items + block.Capacity)) {
                    const uint32_t idx = static_cast<uint32_t>(item_ptr - block.Items);
                    item_ptr->nextFreeIdx = block.FirstFreeIdx;
                    block.FirstFreeIdx = idx;
                    return;
                }
            }

            throw std::out_of_range("Pointer does not belong to this pool!");
        }

    };

    template<typename T, typename Alloc>
    static void VectorInsert(std::vector<T, Alloc>& vector, size_t idx, T&& item) {
        vector.insert(vector.begin() + idx, std::forward<T>(item));
    }

    template<typename T, typename Alloc>
    static void VectorRemove(std::vector<T, Alloc>& vector, size_t idx) {
        vector.erase(vector.begin() + idx);
    }

    template<typename CmpLess, typename VectorType>
    static size_t VectorInsertSorted(VectorType& vec, const typename VectorType::value_type&& val) {
        CmpLess comparator;
        const size_t indexToInsert = BinaryFindFirstNotLess(vec.data(), vec.data() + vec.size(), val, CmpLess()) - vector.data();
        VectorInsert(vec, indexToInsert, std::forward<T>(val));
        return indexToInsert;
    }

    template<typename CmpLess, typename IteratorType, typename KeyType>
    IteratorType VectorFindSorted(const IteratorType& begin, const IteratorType& end, const KeyType& value) {
        CmpLess cmp;
        IteratorType iter = BinaryFindFirstNotLess<CmpLess, IteratorType, KeyType>(begin, end, value, cmp);
        if (iter == end || (!cmp(*iter, value) && !cmp(value, *iter))) {
            return iter;
        }
        return end;
    }

}

#endif //!VPR_ALLOC2_COMMON_FUNCTIONALITY_HPP