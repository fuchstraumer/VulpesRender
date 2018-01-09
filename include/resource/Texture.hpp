#pragma once
#ifndef VULPES_VK_TEXTURE_H
#define VULPES_VK_TEXTURE_H
#include "vpr_stdafx.h"
#include "ForwardDecl.hpp"
#include "alloc/Allocation.hpp"
#include "resource/Buffer.hpp"
#include "core/LogicalDevice.hpp"
#include "Image.hpp"
#ifdef GLM_ENABLE_EXPERIMENTAL
#undef GLM_ENABLE_EXPERIMENTAL
#include "gli/gli.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#else
#include "gli/gli.hpp"
#endif
#include "stb/stb_image.h"

namespace vpr {

    enum class textureError {
        INVALID_FILENAME = 1,
        INVALID_FILE_DATA,
        INVALID_TEXTURE_FORMAT,
        ALLOCATION_FAILED,
        TRANSFER_FAILED,
    };
    
    /** An attempt at minimally wrapping loading image data from file, for use when we can't use GLI (i.e, the image is of a more conventional format than DDS/KTX/etc).
    *   Primarily exists to make sure we don't have dangling pointers to image data, along with memory leaks from said image data.
    * \ingroup Resources
    */
    struct texture_2d_t {
        texture_2d_t(const texture_2d_t&) = delete;
        texture_2d_t& operator=(const texture_2d_t&) = delete;

        texture_2d_t(const char* filename) {
            pixels = stbi_load(filename, &x, &y, &channels, 4);
        }

        ~texture_2d_t() {
            if(pixels) {
                stbi_image_free(pixels);
            }
        }

        texture_2d_t(texture_2d_t&& other) : pixels(std::move(other.pixels)), x(std::move(other.x)), y(std::move(other.y)), channels(std::move(other.channels)) {
            other.pixels = nullptr;
        }

        texture_2d_t& operator=(texture_2d_t&& other) {
            pixels = std::move(other.pixels);
            x = std::move(other.x);
            y = std::move(other.y);
            channels = std::move(other.channels);
            other.pixels = nullptr;
            return *this;
        }

        size_t size() const noexcept {
            return x * y * channels * sizeof(uint8_t);
        }

        stbi_uc* data() noexcept {
            return pixels;
        }

        VkExtent2D extent() const noexcept {
            return VkExtent2D{ static_cast<uint32_t>(x), static_cast<uint32_t>(y) };
        }

        int levels() const noexcept {
            return 1;
        }

        int layers() const noexcept {
            return 1;
        }

        int x, y;
        int channels;
        stbi_uc* pixels;
    };

    /** A templated wrapper around Vulkan texture objects, which require quite a bit of boilerplate code. The templated parameter decides the underlying type 
    *   of the texture object, along with how we load it to file and how upload it ot the device. 
    *   The template parameter can be the following GLI types, besides the texture_2d_t type specified by this library:
    *   - gli::texture2d: 2D texture in a compressed data format that requires specialized loading.
    *   - gli::texture_cube: same as above in terms of specialized image data, but specifies a cubemap with all 6 face images saved into one texture file.
    *   - gli::texture2d_array: same as previous types in terms of image data, but specifies a 2D array of textures packed into one file. The layer count 
    *                           is found during the loading process, but this is stored internally so the layer count must be known by other elements (e.g, shaders)
    *                           that intend to index the texture object.
    *   \ingroup Resources
    */
    template<typename texture_type>
    class Texture : public Image {
    public:

        Texture(const Device* _parent, const VkImageUsageFlags& flags = VK_IMAGE_USAGE_SAMPLED_BIT);

        ~Texture();

        /** Having to use texture_format an unfortunate artifact of the range of formats supported by GLI and Vulkan, and the mismatch
        *   in specification, quantity, and naming between the two. When in doubt, set a breakpoint after loading the texture in question
        *   from file, and check the gli object's "Format" field.
        */
        void CreateFromFile(const char* filename, const VkFormat& texture_format);
        /** This particular method for loading from file uses STB, and attempts to find the correct texture format based on the given file.
        *   Usually just ends up being RGBA8: for compressed textures, use the other method as GLI is required for loading compressed images.
        */
        void CreateFromFile(const char* filename);
        /** If the other file-based methods cannot work for an intended use case, use this method along with the specified buffer info to setup
        *   a texture object appropriately. 
        */
        void CreateFromBuffer(VkBuffer&& staging_buffer, const VkFormat& texture_format, const std::vector<VkBufferImageCopy>& copy_info);
        /** Creates an empty texture suitable for use as an object to write to in compute/graphics renderpasses.
        */
        void CreateEmptyTexture(const VkFormat& texture_format, const uint32_t& width, const uint32_t& height);

        void TransferToDevice(const VkCommandBuffer& transfer_cmd_buffer) const;
        void CreateSampler(const VkSamplerCreateInfo& create_info);

        const VkDescriptorImageInfo& GetDescriptor() const noexcept;
        const VkSampler& Sampler() const noexcept;

        uint32_t Width = 0, Height = 0, Depth = 0;

    private:

        void setDescriptorInfo() const;
        // creates backing handle and gets memory for the texture proper
        void createTexture();
        void createView();
        void createSampler();
        texture_type loadTextureDataFromFile(const char* filename);
        void updateTextureParameters(const texture_type& texture_data);
        void createCopyInformation(const texture_type& texture_data);
        void copyFromFileToStaging(const char* filename);

        VkSampler sampler;

        VkBuffer stagingBuffer;
        Allocation stagingMemory;

        uint32_t mipLevels = 0, layerCount = 0;
        mutable bool descriptorInfoSet = false;
        mutable VkDescriptorImageInfo texDescriptor;
        std::vector<VkBufferImageCopy> copyInfo;
    };

}

#include "Texture.inl"

#endif // !VULPES_VK_TEXTURE_H
