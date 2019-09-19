#pragma once
#ifndef VULPES_VK_FORWARD_DECL_HPP
#define VULPES_VK_FORWARD_DECL_HPP

struct SDL_Window;
struct GLFWwindow;

namespace vpr {
    class Allocation;
    class Allocator;
    class MemoryBlock;
    struct Suballocation;
    class AllocationCollection;
    struct AllocationRequirements;
    class Instance;
    class PhysicalDevice;
    class Device;
    class Buffer;
    class Image;
    class Swapchain;
    class ShaderModule;
    class Framebuffer;
    class Renderpass;
    class CommandPool;
    class GraphicsPipeline;
    class PipelineCache;
    class DescriptorSet;
    class DescriptorPool;
    class PipelineLayout;
    class SurfaceKHR;
    class DescriptorSetLayout;
    class Sampler;
    class Fence;
    class Semaphore;
    class Queue;
    struct GraphicsPipelineInfo;
    struct VprExtensionPack;
}

#endif // !VULPES_VK_FORWARD_DECL_H
