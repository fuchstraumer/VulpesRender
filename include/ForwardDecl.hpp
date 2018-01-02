#pragma once
#ifndef VULPES_VK_FORWARD_DECL_H
#define VULPES_VK_FORWARD_DECL_H
namespace vpr {
    struct Allocation;
    class Allocator;
    class MemoryBlock;
    struct Suballocation;
    class Instance;
    class InstanceGLFW;
    class PhysicalDevice;
    class Device;
    class Buffer;
    class Image;
    class Swapchain;
    class ShaderModule;
    class DeviceMemory;
    class Pipeline;
    class Framebuffer;
    class Renderpass;
    class CommandPool;
    class TransferPool;
    class GraphicsPipeline;
    class DeferredPass;
    class PipelineCache;
    class DescriptorSet;
    class DescriptorPool;
    class PipelineLayout;
    class DepthStencil;
    class Multisampling;
    class SurfaceKHR;
    class DescriptorSetLayout;
    template<typename texture_type>
    class Texture;
}
#endif // !VULPES_VK_FORWARD_DECL_H
