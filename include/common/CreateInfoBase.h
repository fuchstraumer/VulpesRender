#pragma once
#ifndef VULPES_VK_CREATE_INFO_BASE_H
#define VULPES_VK_CREATE_INFO_BASE_H
#include "vulkan/vulkan.h"
/*
	
	Includes a number of predefined simpled create info structs.

	All constexpr, and most will need to have the rest of their members
	set before use but this removes the need for the annoying stype enums

*/

namespace vulpes {

	// base application info struct
	constexpr VkApplicationInfo vk_base_application_info{
		VK_STRUCTURE_TYPE_APPLICATION_INFO,
		nullptr,
		"DiamondDogs-VK",
		VK_MAKE_VERSION(0, 1, 0),
		"DiamondDogs",
		VK_MAKE_VERSION(0, 1, 0),
		VK_API_VERSION_1_0
	};

	// base instance creation struct
	constexpr VkInstanceCreateInfo vk_base_instance_info{
		VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		nullptr,					// used for extensions when not nullptr
		0,							// VK_INSTANCE_CREATE_FLAGS, currently unused
		&vk_base_application_info,	// application info
	};

	// Debug report callback info
	static VkDebugReportCallbackCreateInfoEXT vk_debug_callback_create_info_base{
		VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
		nullptr,
		0,
		nullptr,
		nullptr,
	};

	// Queue object create info
	constexpr float default_priorityf = 1.0f;
	constexpr VkDeviceQueueCreateInfo vk_device_queue_create_info_base{
		VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		nullptr,
		0,
		std::numeric_limits<uint32_t>::max(), // invalid, make sure to set this.
		1, // number of queues
		nullptr // float* of queue priorities, defaulting by leaving this to nullptr is fine
	};

	// Logical device create info, base
	constexpr VkDeviceCreateInfo vk_device_create_info_base{
		VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		nullptr,
		0,
		1,
		&vk_device_queue_create_info_base
	};

	// Base buffer create info
	constexpr VkBufferCreateInfo vk_buffer_create_info_base{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		nullptr,
		0,
		0,
		VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr
	};

	// Base memory allocation info
	constexpr VkMemoryAllocateInfo vk_allocation_info_base{
		VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		nullptr,
	};

	// default mapped memory range
	constexpr VkMappedMemoryRange vk_mapped_memory_base{
		VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
		nullptr,
		VK_NULL_HANDLE,
		0,
		0,
	};

	// default command pool create info
	constexpr VkCommandPoolCreateInfo vk_command_pool_info_base{
		VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		nullptr,
		0,
		std::numeric_limits<uint32_t>::max(),
	};

	// default command buffer create info
	constexpr VkCommandBufferAllocateInfo vk_command_buffer_allocate_info_base{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		nullptr,
		VK_NULL_HANDLE,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		0,
	};

	constexpr VkCommandBufferBeginInfo vk_command_buffer_begin_info_base{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		0,
		nullptr,
	};

	constexpr VkCommandBufferInheritanceInfo vk_command_buffer_inheritance_info_base{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		nullptr,
		VK_NULL_HANDLE,
		0,
		VK_NULL_HANDLE,
		VK_FALSE,
		0,
		0,
	};

	constexpr VkRenderPassBeginInfo vk_renderpass_begin_info_base{
		VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, 
		nullptr,
		VK_NULL_HANDLE, 
		VK_NULL_HANDLE, 
		VkRect2D { },
		0,
		nullptr,
	};

	// default command submission info
	constexpr VkSubmitInfo vk_submit_info_base{
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		nullptr,
		0,
		nullptr,
		nullptr,
		0,
		nullptr,
		0,
		nullptr,
	};

	// default fence create info
	constexpr VkFenceCreateInfo vk_fence_create_info_base{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		nullptr,
		0,
	};

	// default image create info
	constexpr VkImageCreateInfo vk_image_create_info_base{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		nullptr,
		0,
		VK_IMAGE_TYPE_2D,
		VK_FORMAT_UNDEFINED,
		VkExtent3D{},
		0,
		0,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_TILING_LINEAR,
		0,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		VK_IMAGE_LAYOUT_UNDEFINED,
	};

	constexpr VkSamplerCreateInfo vk_sampler_create_info_base{
		VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		nullptr,
		0,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_SAMPLER_MIPMAP_MODE_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		0.0f,
		VK_TRUE,
		8.0f,
		VK_FALSE,
		VK_COMPARE_OP_NEVER,
		0.0f,
		0.0f,
		VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
		VK_FALSE,
	};

	constexpr VkImageMemoryBarrier vk_image_mem_barrier_base{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr,
		0,
		0,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_UNDEFINED,
		std::numeric_limits<uint32_t>::max(),
		std::numeric_limits<uint32_t>::max(),
		VK_NULL_HANDLE,
		VK_NULL_HANDLE,
	};

	// This should be tuned at least slightly before being used: format at the least needs to be set.
	// Component swizzles and last field may also change between image views.
	constexpr VkImageViewCreateInfo vk_image_view_create_info_base{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		nullptr,
		0,
		VK_NULL_HANDLE,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_FORMAT_UNDEFINED,
		{ VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A },
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
	};

	constexpr VkRenderPassCreateInfo vk_render_pass_create_info_base{
		VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr,
		0,
		nullptr,
		0,
		nullptr,
	};

	constexpr VkFramebufferCreateInfo vk_framebuffer_create_info_base{
		VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		nullptr,
		0,
		VK_NULL_HANDLE,
		0,
		nullptr,
		0,
		0,
		0,
	};

	constexpr VkSwapchainCreateInfoKHR vk_swapchain_create_info_base{
		VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		nullptr, 
		0,
		VK_NULL_HANDLE,
		0,
		VK_FORMAT_UNDEFINED,
		VK_COLOR_SPACE_PASS_THROUGH_EXT,
		VkExtent2D{},
		0,
		0,
		VK_SHARING_MODE_EXCLUSIVE,
		0,
		nullptr,
		VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_PRESENT_MODE_FIFO_KHR,
		VK_FALSE,
		VK_NULL_HANDLE,
	};

	constexpr VkPresentInfoKHR vk_present_info_base{
		VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		nullptr,
		0,
		nullptr,
		0,
		nullptr,
		nullptr,
		nullptr,
	};

	constexpr VkShaderModuleCreateInfo vk_shader_module_create_info_base {
		VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr,
	};

	constexpr VkPipelineShaderStageCreateInfo vk_pipeline_shader_stage_create_info_base{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		nullptr,
		0,
		VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM,
		VK_NULL_HANDLE,
		nullptr,
		nullptr
	};

	/*
		Bunch of pipeline create info's follow
	*/

	constexpr VkPipelineInputAssemblyStateCreateInfo vk_pipeline_input_assembly_create_info_base{
		VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_FALSE,
	};

	constexpr VkPipelineRasterizationStateCreateInfo vk_pipeline_rasterization_create_info_base{
		VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_FALSE, // Don't want to do this, discards certain fragments. Useful for shadow maps, mainly.
		VK_FALSE, // If set to true this stage is disabled.
		VK_POLYGON_MODE_FILL, 
		VK_CULL_MODE_BACK_BIT,
		VK_FRONT_FACE_CLOCKWISE,
		VK_FALSE,
		0.0f,
		0.0f,
		0.0f,
		1.0f,
	};

	

	constexpr VkPipelineTessellationStateCreateInfo vk_pipeline_tesselation_state_create_info_base{
		VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		nullptr,
		0,
		0,
	};

	constexpr VkPipelineColorBlendAttachmentState vk_pipeline_color_blend_attachment_info_base{
		VK_TRUE,
		VK_BLEND_FACTOR_SRC_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		VK_BLEND_OP_ADD,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_ZERO,
		VK_BLEND_OP_ADD,
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	constexpr VkPipelineColorBlendStateCreateInfo vk_pipeline_color_blend_create_info_base{
		VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_FALSE,
		VK_LOGIC_OP_COPY,
		1,
		&vk_pipeline_color_blend_attachment_info_base,
		{0.0f, 0.0f, 0.0f, 0.0f},
	};

	constexpr VkPipelineDepthStencilStateCreateInfo vk_pipeline_depth_stencil_create_info_base{
		VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_TRUE,
		VK_TRUE,
		VK_COMPARE_OP_LESS,
		VK_FALSE,
		VK_FALSE,
		{},
		{},
		0.0f, 
		1.0f,
	};

	constexpr VkViewport vk_default_viewport{
		0, 0, // X, Y
		1024, 720, // WIDTH, HEIGHT
		0.0f, 1.0f // MIN_DEPTH, MAX_DEPTH
	};

	constexpr VkRect2D vk_default_viewport_scissor{
		0, 0, // Offset from zero to cut fragments at
		VkExtent2D{static_cast<uint32_t>(vk_default_viewport.width), static_cast<uint32_t>(vk_default_viewport.height)}, // Extents to draw in, copied from default viewport.
	};

	constexpr VkPipelineViewportStateCreateInfo vk_pipeline_viewport_create_info_base{
		VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		nullptr,
		0,
		1, 
		nullptr,
		1, 
		nullptr,
	};

	constexpr VkPipelineMultisampleStateCreateInfo vk_pipeline_multisample_create_info_base{
		VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		nullptr,
		0,
		VK_SAMPLE_COUNT_8_BIT,
		VK_TRUE,
		0.25f, 
		nullptr, 
		VK_FALSE,
		VK_FALSE,
	};

	constexpr VkPipelineLayoutCreateInfo vk_pipeline_layout_create_info_base{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr,
		0,
		nullptr
	};

	constexpr VkPipelineDynamicStateCreateInfo vk_pipeline_dynamic_state_create_info_base{
		VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr,
	};

	constexpr VkGraphicsPipelineCreateInfo vk_graphics_pipeline_create_info_base{
		VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		VK_NULL_HANDLE,
		VK_NULL_HANDLE,
		-1,
		VK_NULL_HANDLE,
		-1,
	};

	constexpr VkPipelineVertexInputStateCreateInfo vk_pipeline_vertex_input_state_create_info_base{
		VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr,
		0,
		nullptr,
	};

	constexpr VkAttachmentDescription vk_attachment_description_base{
		0,
		VK_FORMAT_UNDEFINED,
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_UNDEFINED,
	};

	constexpr VkSubpassDescription vk_subpass_description_base{
		0,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		0,
		nullptr,
		0,
		nullptr,
		nullptr,
		nullptr,
		0,
		nullptr
	};

	constexpr VkPipelineCacheCreateInfo vk_pipeline_cache_create_info_base{
		VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr,
	};

	constexpr VkImageMemoryBarrier vk_image_memory_barrier_base{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		nullptr,
		0,
		0,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		VK_NULL_HANDLE,
		VK_NULL_HANDLE,
	};

	constexpr VkDescriptorSetLayoutCreateInfo vk_descriptor_set_layout_create_info_base {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		0,
		nullptr,
	};

	constexpr VkDescriptorPoolCreateInfo vk_descriptor_pool_create_info_base {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		nullptr,
		0,
		1,
		0,
		nullptr,
	};

	constexpr VkDescriptorSetAllocateInfo vk_descriptor_set_alloc_info_base {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		nullptr,
		VK_NULL_HANDLE,
		1,
		nullptr,
	};


	constexpr VkComputePipelineCreateInfo vk_compute_pipeline_create_info_base{
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		nullptr,
		0,
		VkPipelineShaderStageCreateInfo{},
		VK_NULL_HANDLE,
		VK_NULL_HANDLE,
		std::numeric_limits<uint32_t>::max(),
	};
	
	constexpr VkBufferMemoryBarrier vk_buffer_memory_barrier_info_base{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		nullptr,
		0,
		0,
		std::numeric_limits<uint32_t>::max(),
		std::numeric_limits<uint32_t>::max(),
		VK_NULL_HANDLE,
		0,
		0,
	};

	constexpr VkDebugMarkerObjectNameInfoEXT vk_debug_marker_object_name_info_base{
		VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT,
		nullptr,
	};

}

#endif // !VULPES_VK_CREATE_INFO_BASE_H
