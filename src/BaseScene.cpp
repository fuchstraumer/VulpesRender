#include "vpr_stdafx.h"
#include "BaseScene.h"

#include "core/Instance.h"
#include "core/LogicalDevice.h"
#include "core/PhysicalDevice.h"
#include "render/Swapchain.h"
#include "render/Renderpass.h"
#include "render/Framebuffer.h"
#include "command/CommandPool.h"
#include "render/DepthStencil.h"
#include "util/AABB.h"

namespace vulpes {


	vulpes::BaseScene::BaseScene(const size_t& num_secondary_buffers, const uint32_t& _width, const uint32_t& _height) : width(_width), height(_height) {

		VkInstanceCreateInfo create_info = vk_base_instance_info;
		instance = std::make_unique<InstanceGLFW>(create_info, false);
		glfwSetWindowUserPointer(instance->Window, this);
		instance->SetupPhysicalDevices();
		instance->SetupSurface();

		device = std::make_unique<Device>(instance.get(), instance->physicalDevice);

		swapchain = std::make_unique<Swapchain>();
		swapchain->Init(instance.get(), instance->physicalDevice, device.get());

		CreateCommandPools(num_secondary_buffers);
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

		vkDestroySemaphore(device->vkHandle(), semaphores[1], nullptr);
		vkDestroySemaphore(device->vkHandle(), semaphores[0], nullptr);

	}

	void BaseScene::UpdateMouseActions() {

		ImGuiIO& io = ImGui::GetIO();
		if (!ImGui::IsAnyItemHovered()) {
			// use else-if to only allow one drag at a time.
			if (ImGui::IsMouseDragging(0)) {
				Instance::MouseDrag(0, io.MousePos.x, io.MousePos.y);
				ImGui::ResetMouseDragDelta(0);
			}
			else if (ImGui::IsMouseDragging(1)) {
				Instance::MouseDrag(1, io.MousePos.x, io.MousePos.y);
				ImGui::ResetMouseDragDelta(1);
			}

			if (ImGui::IsMouseDown(0) && !ImGui::IsMouseDragging(0)) {
				Instance::MouseDown(0, io.MouseClickedPos[0].x, io.MouseClickedPos[0].y);
			}

			if (ImGui::IsMouseDown(1) && !ImGui::IsMouseDragging(1)) {
				Instance::MouseDown(1, io.MouseClickedPos[1].x, io.MouseClickedPos[1].y);
			}

			if (ImGui::IsMouseReleased(0)) {
				Instance::MouseUp(0, io.MousePos.x, io.MousePos.y);
			}
		}
	}

	void vulpes::BaseScene::CreateCommandPools(const size_t& num_secondary_buffers) {

		VkCommandPoolCreateInfo pool_info = vk_command_pool_info_base;
		pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = device->QueueFamilyIndices.Graphics;
		graphicsPool = std::make_unique<CommandPool>(device.get(), pool_info, true);

		VkCommandBufferAllocateInfo alloc_info = vk_command_buffer_allocate_info_base;
		graphicsPool->AllocateCmdBuffers(swapchain->ImageCount, alloc_info);

		pool_info.queueFamilyIndex = device->QueueFamilyIndices.Graphics;
		transferPool = std::make_unique<TransferPool>(device.get());
		transferPool->AllocateCmdBuffers(1);

		pool_info.queueFamilyIndex = device->QueueFamilyIndices.Graphics;
		secondaryPool = std::make_unique<CommandPool>(device.get(), pool_info, false);
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		secondaryPool->AllocateCmdBuffers(swapchain->ImageCount * static_cast<uint32_t>(num_secondary_buffers), alloc_info);

	}


	void BaseScene::createRenderTargetAttachment() {

		attachmentDescriptions[0] = vk_attachment_description_base;
		attachmentDescriptions[0].format = swapchain->ColorFormat;
		attachmentDescriptions[0].samples = Multisampling::SampleCount;
		attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	}

	void BaseScene::createResolveAttachment() {

		attachmentDescriptions[1] = vk_attachment_description_base;
		attachmentDescriptions[1].format = swapchain->ColorFormat;
		attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	}

	void BaseScene::createMultisampleDepthAttachment() {

		attachmentDescriptions[2] = vk_attachment_description_base;
		attachmentDescriptions[2].format = device->FindDepthFormat();
		attachmentDescriptions[2].samples = Multisampling::SampleCount;
		attachmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	}

	void BaseScene::createResolveDepthAttachment() {

		attachmentDescriptions[3] = vk_attachment_description_base;
		attachmentDescriptions[3].format = device->FindDepthFormat();
		attachmentDescriptions[3].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescriptions[3].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescriptions[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptions[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptions[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptions[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	}

	void BaseScene::createAttachmentReferences() {

		attachmentReferences[0] = VkAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		attachmentReferences[1] = VkAttachmentReference{ 2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		attachmentReferences[2] = VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	}

	VkSubpassDescription BaseScene::createSubpassDescription() {

		VkSubpassDescription result = vk_subpass_description_base;
		result.colorAttachmentCount = 1;
		result.pColorAttachments = &attachmentReferences[0];
		result.pResolveAttachments = &attachmentReferences[2];
		result.pDepthStencilAttachment = &attachmentReferences[1];
		return result;

	}

	void BaseScene::createSubpassDependencies() {
		
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		subpassDependencies[1].srcSubpass = 0;
		subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	}

	void vulpes::BaseScene::SetupRenderpass(const VkSampleCountFlagBits& sample_count) {

		msaa = std::make_unique<Multisampling>(device.get(), swapchain.get(), sample_count, swapchain->Extent.width, swapchain->Extent.height);

		createRenderTargetAttachment();
		createResolveAttachment();
		createMultisampleDepthAttachment();
		createResolveDepthAttachment();

		createAttachmentReferences();
		VkSubpassDescription subpass_descr = createSubpassDescription();
		createSubpassDependencies();

		VkRenderPassCreateInfo renderpass_create_info = vk_render_pass_create_info_base;
		renderpass_create_info.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
		renderpass_create_info.pAttachments = attachmentDescriptions.data();
		renderpass_create_info.subpassCount = 1;
		renderpass_create_info.pSubpasses = &subpass_descr;
		renderpass_create_info.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
		renderpass_create_info.pDependencies = subpassDependencies.data();

		renderPass = std::make_unique<Renderpass>(device.get(), renderpass_create_info);

	}

	void vulpes::BaseScene::SetupDepthStencil() {

		VkQueue depth_queue = device->GraphicsQueue(0);
		depthStencil = std::make_unique<DepthStencil>(device.get(), VkExtent3D{ swapchain->Extent.width, swapchain->Extent.height, 1 }, graphicsPool.get(), depth_queue);

	}

	void vulpes::BaseScene::SetupFramebuffers() {

		std::array<VkImageView, 4> attachments{ msaa->ColorBufferMS->View(), VK_NULL_HANDLE, msaa->DepthBufferMS->View(), depthStencil->View() };
		VkFramebufferCreateInfo framebuffer_create_info = vk_framebuffer_create_info_base;
		framebuffer_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebuffer_create_info.pAttachments = attachments.data();
		framebuffer_create_info.renderPass = renderPass->vkHandle();
		framebuffer_create_info.width = swapchain->Extent.width;
		framebuffer_create_info.height = swapchain->Extent.height;
		framebuffer_create_info.layers = 1;

		for (uint32_t i = 0; i < swapchain->ImageCount; ++i) {
			attachments[1] = swapchain->ImageViews[i];
			VkFramebuffer new_fbuff;
			VkResult result = vkCreateFramebuffer(device->vkHandle(), &framebuffer_create_info, nullptr, &new_fbuff);
			VkAssert(result);
			framebuffers.push_back(std::move(new_fbuff));
		}

	}

	void vulpes::BaseScene::RecreateSwapchain() {

		// First wait to make sure nothing is in use.
		vkDeviceWaitIdle(device->vkHandle());

		depthStencil.reset();
		framebuffers.clear();
		framebuffers.shrink_to_fit();

		transferPool.reset();
		size_t num_secondary_buffers = secondaryPool->size();
		secondaryPool.reset();
		graphicsPool.reset();

		WindowResized();
		msaa->ColorBufferMS.reset();
		msaa->DepthBufferMS.reset();
		msaa.reset();
		renderPass->Destroy();
		renderPass.reset();
		swapchain->Recreate();

		/*
			Done destroying resources, recreate resources and objects now
		*/
		
		CreateCommandPools(num_secondary_buffers);
		SetupRenderpass();
		SetupDepthStencil();
		SetupFramebuffers();
		RecreateObjects();
		vkDeviceWaitIdle(device->vkHandle());

	}

	float vulpes::BaseScene::GetFrameTime() {
		return frameTime;
	}
}