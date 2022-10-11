#include "Instance.hpp"
#include "LogicalDevice.hpp"
#include "PhysicalDevice.hpp"
#include "Swapchain.hpp"
#include "SurfaceKHR.hpp"
#include <memory>
#include <vector>
#include <cstdint>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#if defined(_WIN32)
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include "GLFW/glfw3native.h"
#endif

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

constexpr static const uint32_t triangleVertShaderSPV[349] =
{
    0x07230203,0x00010000,0x00080007,0x0000002c,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0009000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000a,0x0000001e,0x00000029,
    0x0000002a,0x00030003,0x00000002,0x000001c2,0x00090004,0x415f4c47,0x735f4252,0x72617065,
    0x5f657461,0x64616873,0x6f5f7265,0x63656a62,0x00007374,0x00040005,0x00000004,0x6e69616d,
    0x00000000,0x00060005,0x00000008,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,
    0x00000008,0x00000000,0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000000a,0x00000000,
    0x00040005,0x0000000e,0x62755f5f,0x005f5f6f,0x00050006,0x0000000e,0x00000000,0x65646f6d,
    0x0000006c,0x00050006,0x0000000e,0x00000001,0x77656976,0x00000000,0x00060006,0x0000000e,
    0x00000002,0x6a6f7270,0x69746365,0x00006e6f,0x00030005,0x00000010,0x006f6275,0x00050005,
    0x0000001e,0x69736f70,0x6e6f6974,0x00000000,0x00040005,0x00000029,0x6c6f4376,0x0000726f,
    0x00040005,0x0000002a,0x6f6c6f63,0x00000072,0x00050048,0x00000008,0x00000000,0x0000000b,
    0x00000000,0x00030047,0x00000008,0x00000002,0x00040048,0x0000000e,0x00000000,0x00000005,
    0x00050048,0x0000000e,0x00000000,0x00000023,0x00000000,0x00050048,0x0000000e,0x00000000,
    0x00000007,0x00000010,0x00040048,0x0000000e,0x00000001,0x00000005,0x00050048,0x0000000e,
    0x00000001,0x00000023,0x00000040,0x00050048,0x0000000e,0x00000001,0x00000007,0x00000010,
    0x00040048,0x0000000e,0x00000002,0x00000005,0x00050048,0x0000000e,0x00000002,0x00000023,
    0x00000080,0x00050048,0x0000000e,0x00000002,0x00000007,0x00000010,0x00030047,0x0000000e,
    0x00000002,0x00040047,0x00000010,0x00000022,0x00000000,0x00040047,0x00000010,0x00000021,
    0x00000000,0x00040047,0x0000001e,0x0000001e,0x00000000,0x00040047,0x00000029,0x0000001e,
    0x00000000,0x00040047,0x0000002a,0x0000001e,0x00000001,0x00020013,0x00000002,0x00030021,
    0x00000003,0x00000002,0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,
    0x00000004,0x0003001e,0x00000008,0x00000007,0x00040020,0x00000009,0x00000003,0x00000008,
    0x0004003b,0x00000009,0x0000000a,0x00000003,0x00040015,0x0000000b,0x00000020,0x00000001,
    0x0004002b,0x0000000b,0x0000000c,0x00000000,0x00040018,0x0000000d,0x00000007,0x00000004,
    0x0005001e,0x0000000e,0x0000000d,0x0000000d,0x0000000d,0x00040020,0x0000000f,0x00000002,
    0x0000000e,0x0004003b,0x0000000f,0x00000010,0x00000002,0x0004002b,0x0000000b,0x00000011,
    0x00000002,0x00040020,0x00000012,0x00000002,0x0000000d,0x0004002b,0x0000000b,0x00000015,
    0x00000001,0x00040017,0x0000001c,0x00000006,0x00000003,0x00040020,0x0000001d,0x00000001,
    0x0000001c,0x0004003b,0x0000001d,0x0000001e,0x00000001,0x0004002b,0x00000006,0x00000020,
    0x3f800000,0x00040020,0x00000026,0x00000003,0x00000007,0x00040020,0x00000028,0x00000003,
    0x0000001c,0x0004003b,0x00000028,0x00000029,0x00000003,0x0004003b,0x0000001d,0x0000002a,
    0x00000001,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,
    0x00050041,0x00000012,0x00000013,0x00000010,0x00000011,0x0004003d,0x0000000d,0x00000014,
    0x00000013,0x00050041,0x00000012,0x00000016,0x00000010,0x00000015,0x0004003d,0x0000000d,
    0x00000017,0x00000016,0x00050092,0x0000000d,0x00000018,0x00000014,0x00000017,0x00050041,
    0x00000012,0x00000019,0x00000010,0x0000000c,0x0004003d,0x0000000d,0x0000001a,0x00000019,
    0x00050092,0x0000000d,0x0000001b,0x00000018,0x0000001a,0x0004003d,0x0000001c,0x0000001f,
    0x0000001e,0x00050051,0x00000006,0x00000021,0x0000001f,0x00000000,0x00050051,0x00000006,
    0x00000022,0x0000001f,0x00000001,0x00050051,0x00000006,0x00000023,0x0000001f,0x00000002,
    0x00070050,0x00000007,0x00000024,0x00000021,0x00000022,0x00000023,0x00000020,0x00050091,
    0x00000007,0x00000025,0x0000001b,0x00000024,0x00050041,0x00000026,0x00000027,0x0000000a,
    0x0000000c,0x0003003e,0x00000027,0x00000025,0x0004003d,0x0000001c,0x0000002b,0x0000002a,
    0x0003003e,0x00000029,0x0000002b,0x000100fd,0x00010038
};

constexpr static const uint32_t triangleFragShaderSPV[133] =
{
    0x07230203,0x00010000,0x00080007,0x00000013,0x00000000,0x00020011,0x00000001,0x0006000b,
    0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
    0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000c,0x00030010,
    0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00090004,0x415f4c47,0x735f4252,
    0x72617065,0x5f657461,0x64616873,0x6f5f7265,0x63656a62,0x00007374,0x00040005,0x00000004,
    0x6e69616d,0x00000000,0x00050005,0x00000009,0x6b636162,0x66667562,0x00007265,0x00040005,
    0x0000000c,0x6c6f4376,0x0000726f,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,
    0x0000000c,0x0000001e,0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
    0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,
    0x00000008,0x00000003,0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,
    0x0000000a,0x00000006,0x00000003,0x00040020,0x0000000b,0x00000001,0x0000000a,0x0004003b,
    0x0000000b,0x0000000c,0x00000001,0x0004002b,0x00000006,0x0000000e,0x3f800000,0x00050036,
    0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,0x00000005,0x0004003d,0x0000000a,
    0x0000000d,0x0000000c,0x00050051,0x00000006,0x0000000f,0x0000000d,0x00000000,0x00050051,
    0x00000006,0x00000010,0x0000000d,0x00000001,0x00050051,0x00000006,0x00000011,0x0000000d,
    0x00000002,0x00070050,0x00000007,0x00000012,0x0000000f,0x00000010,0x00000011,0x0000000e,
    0x0003003e,0x00000009,0x00000012,0x000100fd,0x00010038
};

enum class TestCode : uint8_t
{
    Failure = 0,
    Success = 1
};

struct PlatformWindow
{
    GLFWwindow* window{ nullptr };
    uint32_t width{ 0 };
    uint32_t height{ 0 };

    TestCode CreatePlatformWindow(uint32_t _width, uint32_t _height, const char* appName)
    {
        width = _width;
        height = _height;
        return initializeWindow(appName);
    }

    TestCode DestroyPlatformWindow()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void PerFrameUpdate()
    {
        glfwPollEvents();
    }

private:

    TestCode initializeWindow(const char* appName)
    {
        int result = glfwInit();
        if (result != GLFW_TRUE)
        {
            return TestCode::Failure;
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        window = glfwCreateWindow(width, height, appName, nullptr, nullptr);
        if (window == nullptr)
        {
            return TestCode::Failure;
        }
        glfwSetWindowUserPointer(window, this);
        return TestCode::Success;
    }
};

struct DepthStencil
{
    DepthStencil() = default;
    DepthStencil(const vpr::Device* device, const vpr::PhysicalDevice* p_device, const vpr::Swapchain* swap);
    ~DepthStencil();
    VkImage Image{ VK_NULL_HANDLE };
    VkDeviceMemory Memory{ VK_NULL_HANDLE };
    VkImageView View{ VK_NULL_HANDLE };
    VkFormat Format;
    VkDevice Parent{ VK_NULL_HANDLE };
};

uint32_t GetMemoryTypeIndex(uint32_t type_bits, VkMemoryPropertyFlags properties, VkPhysicalDeviceMemoryProperties memory_properties);
DepthStencil CreateDepthStencil(const vpr::Device* device, const vpr::PhysicalDevice* physical_device, const vpr::Swapchain* swapchain);

struct TriangleSceneState
{
    std::unique_ptr<vpr::Instance> instance{ nullptr };
    std::unique_ptr<vpr::PhysicalDevice> gpuDevice{ nullptr };
    std::unique_ptr<vpr::Device> logicalDevice{ nullptr };
    std::unique_ptr<vpr::Swapchain> swapchain{ nullptr };
    std::unique_ptr<vpr::SurfaceKHR> surface{ nullptr };
    PlatformWindow platformWindow;
    std::unique_ptr<vpr::DescriptorSetLayout> setLayout{ nullptr };
    std::unique_ptr<vpr::DescriptorSet> descriptorSet{ nullptr };
    std::unique_ptr<vpr::DescriptorPool> descriptorPool{ nullptr };
    std::unique_ptr<vpr::PipelineLayout> pipelineLayout{ nullptr };
    std::unique_ptr<vpr::GraphicsPipeline> graphicsPipeline{ nullptr };
    std::unique_ptr<vpr::ShaderModule> vertexShader{ nullptr };
    std::unique_ptr<vpr::ShaderModule> fragmentShader{ nullptr };
    DepthStencil triangleDepthStencil;
} triangleSceneState;

struct Vertex
{
    float position[3];
    float color[3];
};

struct
{
    VkDeviceMemory memory;
    VkBuffer buffer;
} Vertices;

struct
{
    VkDeviceMemory memory;
    VkBuffer buffer;
    uint32_t count;
} Indices;

struct
{
    VkDeviceMemory memory;
    VkBuffer buffer;
    VkDescriptorBufferInfo descriptor;
} uniformBufferVS;

struct matrix4x4
{
    float data[16];
};

struct
{
    matrix4x4 model;
    matrix4x4 view;
    matrix4x4 projection;
} uboDataVS;

void PrepareDrawBuffers(VkDevice deviceHandle, const vpr::PhysicalDevice* gpu)
{
    static const std::vector<Vertex> baseVertices
    {
        { { 0.5f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { {-0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { { 0.0f,-0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
    };

    static const std::vector<uint16_t> baseIndices{ 0, 1, 2 };
    Indices.count = static_cast<uint32_t>(baseIndices.size());

    VkMemoryAllocateInfo alloc_info
    {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr
    };

    VkBindBufferMemoryInfo bindInfos[2u]
    {
        VkBindBufferMemoryInfo{ VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO, nullptr },
        VkBindBufferMemoryInfo{ VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO, nullptr }
    };

    {
        const VkBufferCreateInfo buffer_info
        {
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            nullptr,
            0,
            static_cast<uint32_t>(baseVertices.size() * sizeof(Vertex)),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            0,
            nullptr
        };

        vkCreateBuffer(deviceHandle, &buffer_info, nullptr, &Vertices.buffer);
        
        VkMemoryRequirements2 memreqs2
        {
            VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
            nullptr,
            VkMemoryRequirements{}
        };

        const VkBufferMemoryRequirementsInfo2 bufferMemReqs
        {
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
            nullptr,
            Vertices.buffer
        };

        vkGetBufferMemoryRequirements2(deviceHandle, &bufferMemReqs, &memreqs2);
        
        alloc_info.allocationSize = memreqs2.memoryRequirements.size;
        alloc_info.memoryTypeIndex = GetMemoryTypeIndex(
            memreqs2.memoryRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            gpu->GetMemoryProperties());
        vkAllocateMemory(deviceHandle, &alloc_info, nullptr, &Vertices.memory);

        void* data = nullptr;
        vkMapMemory(deviceHandle, Vertices.memory, 0, alloc_info.allocationSize, 0, &data);
        memcpy(data, baseVertices.data(), sizeof(Vertex) * baseVertices.size());
        vkUnmapMemory(deviceHandle, Vertices.memory);
        
        bindInfos[0].buffer = Vertices.buffer;
        bindInfos[0].memory = Vertices.memory;
        bindInfos[0].memoryOffset = 0;
        
    }

    {
        const VkBufferCreateInfo buffer_info
        {
            VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            nullptr,
            0,
            static_cast<uint32_t>(baseIndices.size() * sizeof(uint16_t)),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            0,
            nullptr
        };

        vkCreateBuffer(deviceHandle, &buffer_info, nullptr, &Indices.buffer);
        VkMemoryRequirements memreqs{};
        vkGetBufferMemoryRequirements(deviceHandle, Indices.buffer, &memreqs);
        alloc_info.allocationSize = memreqs.size;
        alloc_info.memoryTypeIndex = GetMemoryTypeIndex(
            memreqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            gpu->GetMemoryProperties());
        vkAllocateMemory(deviceHandle, &alloc_info, nullptr, &Indices.memory);

        void* data = nullptr;
        vkMapMemory(deviceHandle, Indices.memory, 0, alloc_info.allocationSize, 0, &data);
        memcpy(data, baseIndices.data(), sizeof(uint16_t) * baseIndices.size());
        vkUnmapMemory(deviceHandle, Indices.memory);

        bindInfos[1].buffer = Indices.buffer;
        bindInfos[1].memory = Indices.memory;
        bindInfos[1].memoryOffset = 0;
    }

    vkBindBufferMemory2(deviceHandle, 2u, bindInfos);

}

void PrepareUniformBuffer(VkDevice deviceHandle, const vpr::PhysicalDevice* gpu)
{
    const VkBufferCreateInfo uboInfo
    {
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr,
        0,
        sizeof(uboDataVS),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr
    };

    vkCreateBuffer(deviceHandle, &uboInfo, nullptr, &uniformBufferVS.buffer);

    VkMemoryRequirements2 memreqs
    {
        VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        nullptr,
        VkMemoryRequirements{}
    };

    VkBufferMemoryRequirementsInfo2 bufferMemReqs
    {
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
        nullptr,
        uniformBufferVS.buffer
    };

    vkGetBufferMemoryRequirements2(deviceHandle, &bufferMemReqs, &memreqs);

}

int main(int numArgs, const char* argv[])
{
    TestCode createWindowResult = triangleSceneState.platformWindow.CreatePlatformWindow(1920, 1080, "VPR_TRIANGLE_TEST");

    return static_cast<int>(createWindowResult);
}

uint32_t GetMemoryTypeIndex(uint32_t type_bits, VkMemoryPropertyFlags properties, VkPhysicalDeviceMemoryProperties memory_properties)
{
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        if ((type_bits & 1) == 1)
        {
            if ((memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        type_bits >>= 1;
    }

}

DepthStencil CreateDepthStencil(const vpr::Device* device, const vpr::PhysicalDevice* physical_device, const vpr::Swapchain* swapchain)
{
    DepthStencil depth_stencil;
    depth_stencil.Format = device->FindDepthFormat();

    const VkImageCreateInfo image_info
    {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        0,
        VK_IMAGE_TYPE_2D,
        depth_stencil.Format,
        VkExtent3D{ swapchain->Extent().width, swapchain->Extent().height, 1 },
        1,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        device->GetFormatTiling(depth_stencil.Format, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT),
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkResult result = VK_SUCCESS;
    result = vkCreateImage(device->vkHandle(), &image_info, nullptr, &depth_stencil.Image);

    VkMemoryAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr };
    VkMemoryRequirements memreqs{};
    vkGetImageMemoryRequirements(device->vkHandle(), depth_stencil.Image, &memreqs);
    alloc_info.allocationSize = memreqs.size;
    alloc_info.memoryTypeIndex =
        GetMemoryTypeIndex(memreqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physical_device->GetMemoryProperties());
    result = vkAllocateMemory(device->vkHandle(), &alloc_info, nullptr, &depth_stencil.Memory);
    result = vkBindImageMemory(device->vkHandle(), depth_stencil.Image, depth_stencil.Memory, 0);

    const VkImageViewCreateInfo view_info
    {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        depth_stencil.Image,
        VK_IMAGE_VIEW_TYPE_2D,
        depth_stencil.Format,
        {},
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }
    };

    result = vkCreateImageView(device->vkHandle(), &view_info, nullptr, &depth_stencil.View);

    return depth_stencil;
}

DepthStencil::DepthStencil(const vpr::Device* device, const vpr::PhysicalDevice* p_device, const vpr::Swapchain* swap)
    : Parent(device->vkHandle())
{
    *this = CreateDepthStencil(device, p_device, swap);
    Parent = device->vkHandle();
}

DepthStencil::~DepthStencil()
{
    if (Parent == VK_NULL_HANDLE)
    {
        return;
    }

    if (Memory != VK_NULL_HANDLE)
    {
        vkFreeMemory(Parent, Memory, nullptr);
    }

    if (View != VK_NULL_HANDLE)
    {
        vkDestroyImageView(Parent, View, nullptr);
    }

    if (Image != VK_NULL_HANDLE)
    {
        vkDestroyImage(Parent, Image, nullptr);
    }
}