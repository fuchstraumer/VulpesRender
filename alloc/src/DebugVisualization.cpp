#include "DebugVisualization.hpp"
#include "AllocatorImpl.hpp"
#include "imgui.h"
#include <forward_list>
namespace vpr {

    enum class memory_attributes {

    };

    static const std::map<SuballocationType, ImVec4> TypeColors{
    { SuballocationType::Free, ImVec4(0.0f, 0.1f, 0.9f, 1.0f) },
    { SuballocationType::Buffer, ImVec4(119.0f / 255.0f, 244.0f / 255.0f, 66.0f / 255.0f, 1.0f) },
    { SuballocationType::ImageLinear, ImVec4(244.0f / 255.0f, 226.0f / 255.0f, 66.0f / 255.0f, 1.0f) },
    { SuballocationType::ImageOptimal, ImVec4(244.0f / 255.0f, 179.0f / 255.0f, 66.0f / 255.0f, 1.0f) },   
    { SuballocationType::Unknown, ImVec4(240.0f / 255.0f, 10.0f / 255.0f, 10.0f / 255.0f, 1.0f) },
    { SuballocationType::ImageUnknown, ImVec4(1.0f, 220.0f / 255.0f, 30.0f / 255.0f, 1.0f) }
    };

    struct SuballocationDrawCommand {
        SuballocationDrawCommand(const ImVec4& color, ImVec2 sz, ImVec2 offset) : Color(color), Size(sz), Offset(offset) {}
        const ImVec4& Color;
        ImVec2 Size;
        ImVec2 Offset;
        void Draw();
    };

    struct MemoryBlockInfo {
        MemoryBlockInfo(const vpr::MemoryBlock& block);
        void Draw();
        void UpdateDrawCommands();
        const std::list<Suballocation>* Suballocations;
        std::forward_list<SuballocationDrawCommand> DrawCommands;
        size_t ParentIdx;
        VkDeviceSize TotalSize;
        VkDeviceSize BufferImageGranularity;
    };

    struct MemoryBlocksGroup {
        MemoryBlocksGroup(const std::vector<std::unique_ptr<vpr::MemoryBlock>>& allocations);
        memory_attributes Attributes;
        std::vector<MemoryBlockInfo> Blocks;
    };

    struct DebugVisualizationImpl {
        DebugVisualizationImpl(Allocator* alloc);
        AllocatorImpl* allocator;
        std::map<AllocatorImpl::AllocationSize, MemoryBlocksGroup> Data;
        void update();
    };

    DebugVisualization::DebugVisualization(Allocator * allocator) {
    }

    DebugVisualizationImpl::DebugVisualizationImpl(Allocator * alloc) : allocator(alloc->impl.get()) {}

    void DebugVisualizationImpl::update() {
        for (const auto& alloc_size_group : allocator->allocations) {
            for (const auto& alloc_group : alloc_size_group.second) {
                Data[alloc_size_group.first] = MemoryBlocksGroup(alloc_group->allocations);
                
            }
        }
    }

    MemoryBlocksGroup::MemoryBlocksGroup(const std::vector<std::unique_ptr<vpr::MemoryBlock>>& allocations) {
        for (auto& block : allocations) {
            Blocks.emplace_back(MemoryBlockInfo(*block));
            Blocks.back().ParentIdx = Blocks.size() - 1;
        }
    }

    MemoryBlockInfo::MemoryBlockInfo(const vpr::MemoryBlock & block) : Suballocations(&block.Suballocations), TotalSize(block.Size) {}

    void MemoryBlockInfo::UpdateDrawCommands() {
        DrawCommands.clear();
        for (auto& alloc : *Suballocations) {
            DrawCommands.emplace_front(SuballocationDrawCommand(TypeColors.at(alloc.Type), ))
        }
    }

}
