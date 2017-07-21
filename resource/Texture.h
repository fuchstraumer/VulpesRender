#pragma once
#ifndef VULPES_VK_TEXTURE_H
#define VULPES_VK_TEXTURE_H
#include "stdafx.h"
#include "engine/renderer/ForwardDecl.h"
#include "engine/renderer/NonCopyable.h"
#include "engine/renderer/resource/Allocator.h"
#include "engine/renderer/resource/Buffer.h"
#include "engine/renderer/core/LogicalDevice.h"
#include "engine/renderer/render/Multisampling.h"
#include "Image.h"

namespace vulpes {

	// TODO: 1D texture, 2D texture array.
	using texture_1d_t = std::integral_constant<int, 0>;
	using texture_2d_t = std::integral_constant<int, 1>;

	template<typename texture_type>
	class Texture : public Image {
	public:

		Texture(const Device* _parent, const VkImageUsageFlags& flags = VK_IMAGE_USAGE_SAMPLED_BIT);

		~Texture() = default;

		// Having to use texture_format an unfortunate artifact of the range of formats supported by GLI and Vulkan, and the mismatch
		// in specification, quantity, and naming between the two. When in doubt, set a breakpoint after loading the texture in question
		// from file, and check the gli object's "Format" field.
		void CreateFromFile(const char* filename, const VkFormat& texture_format);

		void CreateFromBuffer(VkBuffer&& staging_buffer, const VkFormat& texture_format, const std::vector<VkBufferImageCopy>& copy_info);

		void CreateEmptyTexture(const VkFormat& texture_format, const uint32_t& width, const uint32_t& height);

		void TransferToDevice(VkCommandBuffer& transfer_cmd_buffer) const;

		void FreeStagingBuffer();

		VkDescriptorImageInfo GetDescriptor() const noexcept;
		const VkSampler& Sampler() const noexcept;

		uint32_t Width = 0, Height = 0, Depth = 0;

	private:

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

		std::vector<VkBufferImageCopy> copyInfo;
	};

	template<typename texture_type>
	inline Texture<texture_type>::Texture(const Device * _parent, const VkImageUsageFlags & flags) : Image(_parent) {
		createInfo = vk_image_create_info_base;
		createInfo.usage = flags;
	}

	template<typename texture_type>
	inline void Texture<texture_type>::CreateFromFile(const char * filename, const VkFormat& texture_format) {
		format = texture_format;
		copyFromFileToStaging(filename);
		createTexture(); 
		createView();
		createSampler();
	}

	template<typename texture_type>
	inline void Texture<texture_type>::CreateFromBuffer(VkBuffer&& staging_buffer, const VkFormat & texture_format, const std::vector<VkBufferImageCopy>& copy_info) {
		
		stagingBuffer = std::move(staging_buffer);
		format = texture_format;
		copyInfo = copy_info;

		Width = copyInfo.front().imageExtent.width;
		Height = copyInfo.front().imageExtent.height;
		layerCount = copyInfo.front().imageSubresource.layerCount;
		// mipLevels is taken as the quantity of mipmaps PER layer.
		mipLevels = static_cast<uint32_t>(copyInfo.size() / layerCount);
		
		createTexture();
		createView();
		createSampler();

	}

	template<>
	inline void Texture<texture_1d_t>::CreateEmptyTexture(const VkFormat & texture_format, const uint32_t& width, const uint32_t& height) {

		format = texture_format;
		Width = width;
		Height = height;
		layerCount = 1;
		mipLevels = 1;

		createTexture();
		createView();
		createSampler();

	}

	template<typename texture_type>
	inline void Texture<texture_type>::TransferToDevice(VkCommandBuffer & transfer_cmd_buffer) const {

		// Need barriers to transition layout from initial undefined/uninitialized layout to what we'll use in the shader this is for.
		auto barrier0 = Image::GetMemoryBarrier(handle, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		barrier0.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, layerCount };
		auto barrier1 = Image::GetMemoryBarrier(handle, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		barrier1.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, layerCount };

		vkCmdPipelineBarrier(transfer_cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier0);
		assert(!copyInfo.empty());
		vkCmdCopyBufferToImage(transfer_cmd_buffer, stagingBuffer, handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyInfo.size()), copyInfo.data());
		vkCmdPipelineBarrier(transfer_cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier1);

	}

	template<typename texture_type>
	inline void Texture<texture_type>::FreeStagingBuffer() {
		// can now destroy staging object
		parent->vkAllocator->DestroyBuffer(stagingBuffer, stagingMemory);
	}

	template<typename texture_type>
	inline VkDescriptorImageInfo Texture<texture_type>::GetDescriptor() const noexcept {
		return VkDescriptorImageInfo{ sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
	}

	template<typename texture_type>
	inline const VkSampler & Texture<texture_type>::Sampler() const noexcept {
		return sampler;
	}

	template<typename texture_type>
	inline void Texture<texture_type>::copyFromFileToStaging(const char* filename) {

		texture_type texture_data = loadTextureDataFromFile(filename);

		Buffer::CreateStagingBuffer(parent, texture_data.size(), stagingBuffer, stagingMemory);

		VkResult result = VK_SUCCESS;
		void* mapped;
		result = vkMapMemory(parent->vkHandle(), stagingMemory.Memory(), stagingMemory.Offset(), stagingMemory.Size, 0, &mapped);
		VkAssert(result);
		memcpy(mapped, texture_data.data(), texture_data.size());
		vkUnmapMemory(parent->vkHandle(), stagingMemory.Memory());

	}

	template<typename texture_type>
	inline void Texture<texture_type>::updateTextureParameters(const texture_type& texture_data) {
		Width = static_cast<uint32_t>(texture_data.extent().x);
		Height = static_cast<uint32_t>(texture_data.extent().y);
		Depth = 1;
		mipLevels = static_cast<uint32_t>(texture_data.levels());
		layerCount = static_cast<uint32_t>(texture_data.layers());
	}

	template<>
	inline void Texture<texture_1d_t>::createTexture() {

		createInfo.imageType = VK_IMAGE_TYPE_1D;
		createInfo.format = format;
		createInfo.extent = VkExtent3D{ Width, Height, 1 };
		createInfo.mipLevels = mipLevels;
		createInfo.arrayLayers = layerCount;
		createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		createInfo.samples = Multisampling::SampleCount;
		createInfo.tiling = parent->GetFormatTiling(format, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

		Image::CreateImage(handle, memoryAllocation, parent, createInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	}

	template<>
	inline void Texture<texture_1d_t>::createView() {

		VkImageViewCreateInfo view_create_info = vk_image_view_create_info_base;
		view_create_info.subresourceRange.layerCount = layerCount;
		view_create_info.subresourceRange.levelCount = mipLevels;
		view_create_info.image = handle;
		view_create_info.format = format;
		view_create_info.viewType = VK_IMAGE_VIEW_TYPE_1D;

		VkResult result = vkCreateImageView(parent->vkHandle(), &view_create_info, nullptr, &view);
		VkAssert(result);

	}

	template<>
	inline void Texture<gli::texture2d>::createTexture() {

		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.format = format;
		createInfo.extent = VkExtent3D{ Width, Height, 1 };
		createInfo.mipLevels = mipLevels;
		createInfo.arrayLayers = layerCount;
		createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		createInfo.samples = Multisampling::SampleCount;
		createInfo.tiling = parent->GetFormatTiling(format, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

		Image::CreateImage(handle, memoryAllocation, parent, createInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	}

	template<>
	inline void Texture<gli::texture2d>::createView() {
		
		VkImageViewCreateInfo view_create_info = vk_image_view_create_info_base;
		view_create_info.subresourceRange.layerCount = layerCount;
		view_create_info.subresourceRange.levelCount = mipLevels;
		view_create_info.image = handle;
		view_create_info.format = format;
		view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

		VkResult result = vkCreateImageView(parent->vkHandle(), &view_create_info, nullptr, &view);
		VkAssert(result);

	}

	template<typename texture_type>
	inline void Texture<texture_type>::createSampler() {
		VkSamplerCreateInfo sampler_create_info = vk_sampler_create_info_base;
		VkResult result = vkCreateSampler(parent->vkHandle(), &sampler_create_info, nullptr, &sampler);
		VkAssert(result);
	}

	template<>
	inline gli::texture2d Texture<gli::texture2d>::loadTextureDataFromFile(const char* filename) {
		gli::texture2d result = gli::texture2d(gli::load(filename));
		updateTextureParameters(result);
		createCopyInformation(result);
		return std::move(result);
	}

	template<>
	inline void Texture<gli::texture2d>::createCopyInformation(const gli::texture2d& texture_data) {
		assert(mipLevels != 0);
		copyInfo.resize(mipLevels);
		uint32_t offset = 0;
		for (uint32_t i = 0; i < mipLevels; ++i) {
			copyInfo[i] = VkBufferImageCopy{
				offset,
				0,
				0,
				VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, i, 0, layerCount },
				VkOffset3D{ 0, 0, 0 },
				VkExtent3D{ static_cast<uint32_t>(texture_data[i].extent().x), static_cast<uint32_t>(texture_data[i].extent().y), 1 }
			};
			offset += static_cast<uint32_t>(texture_data[i].size());
		}
	}

	template<>
	inline void Texture<gli::texture_cube>::createTexture() {
		
		createInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // Must be set for cubemaps: easy to miss!
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.format = format;
		createInfo.extent = VkExtent3D{ Width, Height, 1 };
		createInfo.mipLevels = mipLevels;
		createInfo.arrayLayers = 6;
		createInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		createInfo.tiling = parent->GetFormatTiling(format, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

		Image::CreateImage(handle, memoryAllocation, parent, createInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	template<> 
	inline void Texture<gli::texture_cube>::createView() {

		VkImageViewCreateInfo view_create_info = vk_image_view_create_info_base;
		view_create_info.subresourceRange.layerCount = layerCount;
		view_create_info.subresourceRange.levelCount = mipLevels;
		view_create_info.image = handle;
		view_create_info.format = format;
		view_create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;

		VkResult result = vkCreateImageView(parent->vkHandle(), &view_create_info, nullptr, &view);
		VkAssert(result);

	}

	template<>
	inline gli::texture_cube Texture<gli::texture_cube>::loadTextureDataFromFile(const char* filename) {
		gli::texture_cube result = gli::texture_cube(gli::load(filename));
		updateTextureParameters(result);
		createCopyInformation(result);
		return std::move(result);
	}

	template<>
	inline void Texture<gli::texture_cube>::createCopyInformation(const gli::texture_cube& texture_data) {
		// Texture cube case is complex: need to create copy info for all mips levels - of each of the six faces.
		// Now set up buffer copy regions for each face and all of its mip levels.
		size_t offset = 0;

		for (uint32_t face_idx = 0; face_idx < 6; ++face_idx) {
			for (uint32_t mip_level = 0; mip_level < mipLevels; ++mip_level) {
				VkBufferImageCopy copy{};
				copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copy.imageSubresource.mipLevel = mip_level;
				copy.imageSubresource.baseArrayLayer = face_idx;
				copy.imageSubresource.layerCount = 1;
				copy.imageExtent.width = static_cast<uint32_t>(texture_data[face_idx][mip_level].extent().x);
				copy.imageExtent.height = static_cast<uint32_t>(texture_data[face_idx][mip_level].extent().y);
				copy.imageExtent.depth = 1;
				copy.bufferOffset = static_cast<uint32_t>(offset);
				copyInfo.push_back(std::move(copy));
				// Increment offset by datasize of last specified copy region
				offset += texture_data[face_idx][mip_level].size();
			}
		}
	}

}

#endif // !VULPES_VK_TEXTURE_H
