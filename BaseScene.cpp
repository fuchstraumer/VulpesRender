#include "stdafx.h"
#include "BaseScene.h"

#include "engine\renderer\core\Instance.h"
#include "engine\renderer\core\LogicalDevice.h"
#include "engine\renderer\core\PhysicalDevice.h"
#include "engine\renderer\render\Swapchain.h"
#include "engine\renderer\render\Renderpass.h"
#include "engine\renderer\render\Framebuffer.h"
#include "engine\renderer\command\CommandPool.h"
#include "engine\renderer\render\DepthStencil.h"
#include "engine\util\AABB.h"

vulpes::BaseScene::BaseScene(const size_t& num_secondary_buffers, const uint32_t& _width, const uint32_t& _height) : width(_width), height(_height) {

	VkInstanceCreateInfo create_info = vk_base_instance_info;
	instance = new InstanceGLFW(create_info, true);
	glfwSetWindowUserPointer(instance->Window, this);
	instance->SetupPhysicalDevices();
	instance->SetupSurface();

	device = new Device(instance, instance->physicalDevice);

	swapchain = new Swapchain();
	swapchain->Init(instance, instance->physicalDevice, device);

	CreateCommandPools(num_secondary_buffers * swapchain->ImageCount);
	SetupRenderpass();
	SetupDepthStencil();

	VkSemaphoreCreateInfo semaphore_info{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0 };
	VkResult result = vkCreateSemaphore(device->vkHandle(), &semaphore_info, nullptr, &semaphores[0]);
	VkAssert(result);
	result = vkCreateSemaphore(device->vkHandle(), &semaphore_info, nullptr, &semaphores[1]);
	VkAssert(result);
}

vulpes::BaseScene::~BaseScene() {
	for (const auto& fbuf : framebuffers) {
		vkDestroyFramebuffer(device->vkHandle(), fbuf, nullptr);
	}
	msaa->ColorBufferMS.reset();
	msaa->DepthBufferMS.reset();
	delete gui;
	delete swapchain;
	delete renderPass;
	delete secondaryPool;
	delete transferPool;
	delete graphicsPool;
	vkDestroySemaphore(device->vkHandle(), semaphores[1], nullptr);
	vkDestroySemaphore(device->vkHandle(), semaphores[0], nullptr);
	delete device;
	delete instance;
}

void vulpes::BaseScene::CreateCommandPools(const size_t& num_secondary_buffers) {
	VkCommandPoolCreateInfo pool_info = vk_command_pool_info_base;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = device->QueueFamilyIndices.Graphics;
	graphicsPool = new CommandPool(device, pool_info, true);

	VkCommandBufferAllocateInfo alloc_info = vk_command_buffer_allocate_info_base;
	graphicsPool->AllocateCmdBuffers(swapchain->ImageCount, alloc_info);

	pool_info.queueFamilyIndex = device->QueueFamilyIndices.Graphics;
	transferPool = new CommandPool(device, pool_info, true);
	transferPool->AllocateCmdBuffers(1);

	pool_info.queueFamilyIndex = device->QueueFamilyIndices.Graphics;
	secondaryPool = new CommandPool(device, pool_info, false);
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	secondaryPool->AllocateCmdBuffers(swapchain->ImageCount * static_cast<uint32_t>(num_secondary_buffers), alloc_info);
}

void vulpes::BaseScene::SetupRenderpass(const VkSampleCountFlagBits& sample_count) {

	std::array<VkAttachmentDescription, 4> attachments;

	VkAttachmentDescription attach_descr = vk_attachment_description_base;
	attach_descr.format = swapchain->ColorFormat;
	attach_descr.samples = VK_SAMPLE_COUNT_1_BIT;
	attach_descr.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attach_descr.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attach_descr.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attach_descr.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attach_descr.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attach_descr.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1] = attach_descr;

	VkAttachmentDescription depth_descr = {
		0,
		device->FindDepthFormat(),
		VK_SAMPLE_COUNT_1_BIT,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	attachments[3] = depth_descr;

	/*
		Setup Multisampling
	*/

	msaa = std::make_unique<Multisampling>(device, swapchain, sample_count, swapchain->Extent.width, swapchain->Extent.height);
	Multisampling::SampleCount = sample_count;

	VkAttachmentDescription msaa_color_attachment = vk_attachment_description_base;
	msaa_color_attachment.format = swapchain->ColorFormat;
	msaa_color_attachment.samples = sample_count;
	msaa_color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	msaa_color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	msaa_color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	msaa_color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	msaa_color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	msaa_color_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attachments[0] = msaa_color_attachment;

	VkAttachmentDescription msaa_depth_attachment = vk_attachment_description_base;
	msaa_depth_attachment.format = device->FindDepthFormat();
	msaa_depth_attachment.samples = sample_count;
	msaa_depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	msaa_depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	msaa_depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	msaa_depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	msaa_depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	msaa_depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachments[2] = msaa_depth_attachment;

	VkAttachmentReference color_ref{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depth_ref{  2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	VkAttachmentReference resolve_ref{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription su_descr = vk_subpass_description_base;
	su_descr.flags = 0;
	su_descr.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	su_descr.inputAttachmentCount = 0;
	su_descr.pInputAttachments = nullptr;
	su_descr.colorAttachmentCount = 1;
	su_descr.pColorAttachments = &color_ref;
	su_descr.pResolveAttachments = &resolve_ref;
	su_descr.pDepthStencilAttachment = &depth_ref;
	su_descr.preserveAttachmentCount = 0;
	su_descr.pPreserveAttachments = nullptr;

	std::array<VkSubpassDependency, 2> subpasses;

	subpasses[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpasses[0].dstSubpass = 0;
	subpasses[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpasses[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpasses[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpasses[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpasses[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	subpasses[1].srcSubpass = 0;
	subpasses[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpasses[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpasses[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpasses[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpasses[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpasses[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


	std::array<VkAttachmentDescription, 2> decriptions{ attach_descr, depth_descr };
	VkRenderPassCreateInfo rp_info = vk_render_pass_create_info_base;
	rp_info.attachmentCount = static_cast<uint32_t>(attachments.size());
	rp_info.pAttachments = attachments.data();
	rp_info.subpassCount = 1;
	rp_info.pSubpasses = &su_descr;
	rp_info.dependencyCount = static_cast<uint32_t>(subpasses.size());
	rp_info.pDependencies = subpasses.data();

	renderPass = new Renderpass(device, rp_info);
}

void vulpes::BaseScene::SetupDepthStencil(){
	VkQueue depth_queue = device->GraphicsQueue(0);
	depthStencil = new DepthStencil(device, VkExtent3D{ swapchain->Extent.width, swapchain->Extent.height, 1 }, transferPool, depth_queue);
}

void vulpes::BaseScene::SetupFramebuffers(){
	for (uint32_t i = 0; i < swapchain->ImageCount; ++i) {
		VkFramebufferCreateInfo f_info = vk_framebuffer_create_info_base;
		f_info.renderPass = *renderPass;
		std::array<VkImageView, 4> attachments{ msaa->ColorBufferMS->View(), swapchain->ImageViews[i], msaa->DepthBufferMS->View(), depthStencil->View() };
		f_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		f_info.pAttachments = attachments.data();
		f_info.width = swapchain->Extent.width;
		f_info.height = swapchain->Extent.height;
		f_info.layers = 1;
		VkFramebuffer new_fbuff;
		VkResult result = vkCreateFramebuffer(device->vkHandle(), &f_info, nullptr, &new_fbuff);
		VkAssert(result);
		framebuffers.push_back(std::move(new_fbuff));
	}
}

void vulpes::BaseScene::RecreateSwapchain(){
	// First wait to make sure nothing is in use.
	vkDeviceWaitIdle(device->vkHandle());

	depthStencil->Destroy();

	framebuffers.clear();
	framebuffers.shrink_to_fit();

	transferPool->FreeCommandBuffers();
	graphicsPool->FreeCommandBuffers();
	size_t num_secondary_buffers = secondaryPool->size();
	secondaryPool->FreeCommandBuffers();

	WindowResized();

	renderPass->Destroy();

	swapchain->Destroy();

	/*
		Done destroying resources, recreate resources and objects now
	*/

	swapchain->Recreate();

	graphicsPool->AllocateCmdBuffers(swapchain->ImageCount);
	transferPool->AllocateCmdBuffers(1);
	secondaryPool->AllocateCmdBuffers(static_cast<uint32_t>(num_secondary_buffers));
	SetupRenderpass();
	RecreateObjects();
	SetupDepthStencil();
	SetupFramebuffers();
	RecordCommands();

	vkDeviceWaitIdle(device->vkHandle());
}

float vulpes::BaseScene::GetFrameTime(){
	return frameTime;
}
