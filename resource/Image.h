#pragma once
#ifndef VULPES_VK_IMAGE_H
#define VULPES_VK_IMAGE_H
#include "stdafx.h"
#include "engine/renderer/ForwardDecl.h"
#include "engine/renderer/NonCopyable.h"
#include "engine/renderer/resource/Allocator.h"
/*

	Wraps the common image creation, transfer, and staging methods. 

	Texture derives from this, and so does DepthStencil.
	
*/

namespace vulpes {

	class Image : public NonCopyable {
	public:

		Image(const Device* parent);

		Image(Image&& other) noexcept;
		Image& operator=(Image&& other) noexcept;

		virtual ~Image();

		void Destroy();

		void Create(const VkImageCreateInfo& create_info, const VkMemoryPropertyFlagBits& memory_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		void Create(const VkExtent3D& extents, const VkFormat& format, const VkImageUsageFlags& usage_flags, const VkImageLayout& init_layout = VK_IMAGE_LAYOUT_PREINITIALIZED);
		
		void CreateView(const VkImageViewCreateInfo& info);
		
		void CreateView(const VkImageAspectFlags& aspect_flags);

		void TransitionLayout(const VkImageLayout& initial, const VkImageLayout& final, CommandPool* cmd, VkQueue& queue);

		static VkImageMemoryBarrier GetMemoryBarrier(const VkImage& image, const VkFormat& img_format, const VkImageLayout& prev, const VkImageLayout& next);

		static void CreateImage(VkImage& dest_image, Allocation& dest_alloc, const Device* parent, const VkExtent3D& extents, const VkFormat& image_format, const VkMemoryPropertyFlags& memory_flags, const VkImageUsageFlags& usage_flags, const VkImageTiling& tiling = VK_IMAGE_TILING_OPTIMAL, const VkImageLayout& init_layout = VK_IMAGE_LAYOUT_PREINITIALIZED);

		static void CreateImage(VkImage& dest_image, Allocation& dest_alloc, const Device* parent, const VkImageCreateInfo& create_info, const VkMemoryPropertyFlags & memory_flags);

		const VkImageCreateInfo& CreateInfo() const noexcept;
		const VkImage& vkHandle() const noexcept;
		const VkImageView& View() const noexcept;
		const VkMappedMemoryRange& Memory() const noexcept;
		virtual VkExtent3D GetExtents() const noexcept;

		VkFormat Format() const noexcept;
		void SetFormat(const VkFormat& format) noexcept;

	protected:

		const Device* parent;
		const VkAllocationCallbacks* allocators = nullptr;

		VkImageCreateInfo createInfo;
		VkImageSubresource subresource;
		VkSubresourceLayout subresourceLayout;

		VkImage handle;
		VkImageView view;
		Allocation memoryAllocation;

		VkImageLayout finalLayout;
		VkExtent3D extents;
		VkFormat format;
		VkImageUsageFlags usageFlags;
		VkDeviceSize imageDataSize;
	};

}

#endif // !VULPES_VK_IMAGE_H
