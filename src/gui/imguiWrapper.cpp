#include "vpr_stdafx.h"
#include "gui/imguiWrapper.hpp"
#include "command/TransferPool.hpp"
#include "core/Instance.hpp"
#include "resource/ShaderModule.hpp"
#include "BaseScene.hpp"

namespace vulpes {

	static std::array<bool, 3> mouse_pressed{ false, false, false };

	imguiWrapper::~imguiWrapper() {
		
		texture.reset();
		vkFreeDescriptorSets(device->vkHandle(), descriptorPool, 1, &descriptorSet);
		vkDestroyDescriptorSetLayout(device->vkHandle(), descriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device->vkHandle(), descriptorPool, nullptr);
		vkDestroyPipelineLayout(device->vkHandle(), pipelineLayout, nullptr);
		cache.reset();
		vbo.reset();
		ebo.reset();

	}

	void imguiWrapper::Init(const Device * dvc, const VkRenderPass & renderpass) {
		
		device = dvc;
		cache = std::make_unique<PipelineCache>(device, static_cast<uint16_t>(typeid(imguiWrapper).hash_code()));

		createResources();
		createDescriptorPools();
		
		createFontTexture();
		
		createDescriptorLayout();
		createPipelineLayout();
		
		allocateDescriptors();
		updateDescriptors();
	
		setupGraphicsPipelineInfo();
		setupGraphicsPipelineCreateInfo(renderpass);

		// This has to be done here, due to scoping issues and auto-destruction rules.
		const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{
			vert->PipelineInfo(),
			frag->PipelineInfo()
		};

		pipelineCreateInfo.pStages = shader_stages.data();
		
		pipeline = std::make_unique<GraphicsPipeline>(device);
		pipeline->Init(pipelineCreateInfo, cache->vkHandle());

	}

	void imguiWrapper::UploadTextureData(TransferPool * transfer_pool) {
		
		// Transfer image data from transfer buffer onto the device.
		auto cmd = transfer_pool->Begin();

		texture->TransferToDevice(cmd);

		
		transfer_pool->Submit();

	}

	void imguiWrapper::NewFrame(Instance* instance, bool update_framegraph) {

        auto* window_ptr = instance->GetWindow();
        updateImguiSpecialKeys();

        auto& io = ImGui::GetIO();
		io.ClipboardUserData = reinterpret_cast<void*>(window_ptr); // required for clipboard funcs to work.
		static double curr_time = 0.0;

		if (Instance::VulpesInstanceConfig.EnableMouseLocking) {
			if (input_handler::Keys[GLFW_KEY_LEFT_ALT]) {
				freeMouse(instance);
				BaseScene::CameraLock = true;
			}
			else {
				captureMouse(instance);
                BaseScene::CameraLock = false;
			}
		}

        

		for (size_t i = 0; i < 3; ++i) {
			io.MouseDown[i] = mouse_pressed[i] || glfwGetMouseButton(window_ptr->glfwWindow(), static_cast<int>(i)) != 0;
			mouse_pressed[i] = false;
		}

		double frame_time = glfwGetTime();
		io.DeltaTime = curr_time > 0.0 ? static_cast<float>(frame_time - curr_time) : (1.0f / 60.0f);
		curr_time = frame_time;

		io.MouseWheel = 0.0f;

		ImGui::NewFrame();
		
	}

	void imguiWrapper::UpdateBuffers() {
		
		validateBuffers();
		updateBufferData();
		
	}


	void imguiWrapper::DrawFrame(VkCommandBuffer & cmd) {
		auto& io = ImGui::GetIO();

		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->vkHandle());

		static const VkDeviceSize offsets[1]{ 0 };
		vkCmdBindVertexBuffers(cmd, 0, 1, &vbo->vkHandle(), offsets);
		vkCmdBindIndexBuffer(cmd, ebo->vkHandle(), 0, VK_INDEX_TYPE_UINT16);

		glm::vec4 push_constants = glm::vec4(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y, -1.0f, -1.0f);
		vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 4, &push_constants);

		VkViewport viewport{ 0, 0, io.DisplaySize.x, io.DisplaySize.y, 0.0f, 1.0f };
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		ImDrawData* draw_data = ImGui::GetDrawData();
		int32_t vtx_offset = 0, idx_offset = 0;
		for (int32_t i = 0; i < draw_data->CmdListsCount; ++i) {
			const auto* cmd_list = draw_data->CmdLists[i];
			for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; ++j) {
				const auto* draw_cmd = &cmd_list->CmdBuffer[j];
				VkRect2D scissor;
				scissor.offset.x = std::max(static_cast<int32_t>(draw_cmd->ClipRect.x), 0);
				scissor.offset.y = std::max(static_cast<int32_t>(draw_cmd->ClipRect.y), 0);
				scissor.extent.width = static_cast<uint32_t>(draw_cmd->ClipRect.z - draw_cmd->ClipRect.x);
				scissor.extent.height = static_cast<uint32_t>(draw_cmd->ClipRect.w - draw_cmd->ClipRect.y + 1);
				vkCmdSetScissor(cmd, 0, 1, &scissor);
				vkCmdDrawIndexed(cmd, draw_cmd->ElemCount, 1, idx_offset, vtx_offset, 0);
				idx_offset += draw_cmd->ElemCount;
			}
			vtx_offset += cmd_list->VtxBuffer.Size;
		}
	}

	void imguiWrapper::createResources() {
		
		vbo = std::make_unique<Buffer>(device);
		ebo = std::make_unique<Buffer>(device);

		vert = std::make_unique<ShaderModule>(device, "../rsrc/shaders/gui/ui.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		frag = std::make_unique<ShaderModule>(device, "../rsrc/shaders/gui/ui.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	}

	void imguiWrapper::createDescriptorPools() {
		
		std::array<VkDescriptorPoolSize, 1> pool_sizes{
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 },
		};

		VkDescriptorPoolCreateInfo pool_info = vk_descriptor_pool_create_info_base;
		pool_info.pPoolSizes = pool_sizes.data();
		pool_info.poolSizeCount = 1;
		pool_info.maxSets = 2;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VkResult result = vkCreateDescriptorPool(device->vkHandle(), &pool_info, nullptr, &descriptorPool);
		VkAssert(result);

	}

	size_t imguiWrapper::loadFontTextureData() {
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->GetTexDataAsRGBA32(&fontTextureData, &imgWidth, &imgHeight);
		return imgWidth * imgHeight * 4 * sizeof(char);
	}

	void imguiWrapper::uploadFontTextureData(const size_t& font_texture_size) {

		Allocation staging_alloc;
		Buffer::CreateStagingBuffer(device, font_texture_size, textureStaging, staging_alloc);

		void* mapped;
		VkResult result = vkMapMemory(device->vkHandle(), staging_alloc.Memory(), staging_alloc.Offset(), staging_alloc.Size, 0, &mapped);
		VkAssert(result);

		memcpy(mapped, fontTextureData, font_texture_size);

		vkUnmapMemory(device->vkHandle(), staging_alloc.Memory());

		VkBufferImageCopy buffer_image_copy{};
		buffer_image_copy.imageSubresource = VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		buffer_image_copy.imageExtent = VkExtent3D{ static_cast<uint32_t>(imgWidth), static_cast<uint32_t>(imgHeight), 1 };
		buffer_image_copy.imageSubresource = VkImageSubresourceLayers{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		stagingToTextureCopy = std::move(buffer_image_copy);

	}

	void imguiWrapper::createFontTexture() {

		// Load texture.
		size_t texture_data_size = loadFontTextureData();

		// Upload pixel data to staging/transfer buffer object
		uploadFontTextureData(texture_data_size);

		// Create texture
		texture = std::make_unique<Texture<gli::texture2d>>(device);
		texture->CreateFromBuffer(std::move(textureStaging), VK_FORMAT_R8G8B8A8_UNORM, std::vector<VkBufferImageCopy>{ stagingToTextureCopy} );
		fontSampler = texture->Sampler();
		fontView = texture->View();

	}

	void imguiWrapper::createDescriptorLayout() {
		
		VkDescriptorSetLayoutBinding layout_binding{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, &fontSampler };
		VkDescriptorSetLayoutCreateInfo layout_info = vk_descriptor_set_layout_create_info_base;
		layout_info.bindingCount = 1;
		layout_info.pBindings = &layout_binding;
		VkResult result = vkCreateDescriptorSetLayout(device->vkHandle(), &layout_info, nullptr, &descriptorSetLayout);
		VkAssert(result);

	}

	void imguiWrapper::createPipelineLayout() {

		VkPushConstantRange push_constant{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 4 };
		VkPipelineLayoutCreateInfo pipeline_info = vk_pipeline_layout_create_info_base;
		pipeline_info.pushConstantRangeCount = 1;
		pipeline_info.pPushConstantRanges = &push_constant;
		pipeline_info.pSetLayouts = &descriptorSetLayout;
		pipeline_info.setLayoutCount = 1;
		VkResult result = vkCreatePipelineLayout(device->vkHandle(), &pipeline_info, nullptr, &pipelineLayout);
		VkAssert(result);

	}

	void imguiWrapper::allocateDescriptors() {

		VkDescriptorSetAllocateInfo set_alloc = vk_descriptor_set_alloc_info_base;
		set_alloc.descriptorPool = descriptorPool;
		set_alloc.descriptorSetCount = 1;
		set_alloc.pSetLayouts = &descriptorSetLayout;
		VkResult result = vkAllocateDescriptorSets(device->vkHandle(), &set_alloc, &descriptorSet);
		VkAssert(result);

	}

	void imguiWrapper::updateDescriptors() {

		VkDescriptorImageInfo font_info;
		font_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		font_info.imageView = fontView;
		font_info.sampler = fontSampler;
		VkWriteDescriptorSet write_set{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, descriptorSet, 0, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &font_info, nullptr, nullptr };
		vkUpdateDescriptorSets(device->vkHandle(), 1, &write_set, 0, nullptr);

	}

	void imguiWrapper::setupGraphicsPipelineInfo() {

		static const VkVertexInputBindingDescription bind_descr{ 0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX };

		static const std::array<VkVertexInputAttributeDescription, 3> attr_descr{
			VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 },
			VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32_SFLOAT,  sizeof(float) * 2 },
			VkVertexInputAttributeDescription{ 2, 0, VK_FORMAT_R8G8B8A8_UNORM, sizeof(float) * 4 }
		};

		pipelineStateInfo.VertexInfo.vertexBindingDescriptionCount = 1;
		pipelineStateInfo.VertexInfo.pVertexBindingDescriptions = &bind_descr;
		pipelineStateInfo.VertexInfo.vertexAttributeDescriptionCount = 3;
		pipelineStateInfo.VertexInfo.pVertexAttributeDescriptions = attr_descr.data();

		static const VkPipelineColorBlendAttachmentState color_blend{
			VK_TRUE,
			VK_BLEND_FACTOR_SRC_ALPHA,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			VK_BLEND_OP_ADD,
			VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			VK_BLEND_FACTOR_ZERO,
			VK_BLEND_OP_ADD,
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		pipelineStateInfo.ColorBlendInfo.attachmentCount = 1;
		pipelineStateInfo.ColorBlendInfo.pAttachments = &color_blend;

		// Set this through dynamic state so we can do it when rendering.
		pipelineStateInfo.DynamicStateInfo.dynamicStateCount = 2;
		static const VkDynamicState states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		pipelineStateInfo.DynamicStateInfo.pDynamicStates = states;

		pipelineStateInfo.DepthStencilInfo.depthTestEnable = VK_TRUE;
		pipelineStateInfo.DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		pipelineStateInfo.RasterizationInfo.cullMode = VK_CULL_MODE_NONE;

		pipelineStateInfo.MultisampleInfo.rasterizationSamples = Instance::VulpesInstanceConfig.MSAA_SampleCount;
		pipelineStateInfo.MultisampleInfo.sampleShadingEnable = Instance::VulpesInstanceConfig.EnableMSAA;

	}

	void imguiWrapper::setupGraphicsPipelineCreateInfo(const VkRenderPass& renderpass) {

		pipelineCreateInfo = vk_graphics_pipeline_create_info_base;
		pipelineCreateInfo.flags = 0;
		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pInputAssemblyState = &pipelineStateInfo.AssemblyInfo;
		pipelineCreateInfo.pTessellationState = nullptr;
		pipelineCreateInfo.pViewportState = &pipelineStateInfo.ViewportInfo;
		pipelineCreateInfo.pRasterizationState = &pipelineStateInfo.RasterizationInfo;
		pipelineCreateInfo.pMultisampleState = &pipelineStateInfo.MultisampleInfo;
		pipelineCreateInfo.pVertexInputState = &pipelineStateInfo.VertexInfo;
		pipelineCreateInfo.pDepthStencilState = &pipelineStateInfo.DepthStencilInfo;
		pipelineCreateInfo.pColorBlendState = &pipelineStateInfo.ColorBlendInfo;
		pipelineCreateInfo.pDynamicState = &pipelineStateInfo.DynamicStateInfo;
		pipelineCreateInfo.layout = pipelineLayout;
		pipelineCreateInfo.renderPass = renderpass;
		pipelineCreateInfo.subpass = 0;
		pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineCreateInfo.basePipelineIndex = -1;

	}

    void imguiWrapper::updateImguiSpecialKeys() noexcept {
        
        ImGuiIO& io = ImGui::GetIO();
        io.KeysDown[io.KeyMap[ImGuiKey_Tab]] = input_handler::Keys[GLFW_KEY_TAB];
        io.KeysDown[io.KeyMap[ImGuiKey_LeftArrow]] = input_handler::Keys[GLFW_KEY_LEFT];
        io.KeysDown[io.KeyMap[ImGuiKey_RightArrow]] = input_handler::Keys[GLFW_KEY_RIGHT];
        io.KeysDown[io.KeyMap[ImGuiKey_UpArrow]] = input_handler::Keys[GLFW_KEY_UP];
        io.KeysDown[io.KeyMap[ImGuiKey_DownArrow]] = input_handler::Keys[GLFW_KEY_DOWN];
        io.KeysDown[io.KeyMap[ImGuiKey_PageUp]] = input_handler::Keys[GLFW_KEY_PAGE_UP];
        io.KeysDown[io.KeyMap[ImGuiKey_PageDown]] = input_handler::Keys[GLFW_KEY_PAGE_DOWN];
        io.KeysDown[io.KeyMap[ImGuiKey_Home]] = input_handler::Keys[GLFW_KEY_HOME];
        io.KeysDown[io.KeyMap[ImGuiKey_End]] = input_handler::Keys[GLFW_KEY_END];
        io.KeysDown[io.KeyMap[ImGuiKey_Delete]] = input_handler::Keys[GLFW_KEY_DELETE];
        io.KeysDown[io.KeyMap[ImGuiKey_Backspace]] = input_handler::Keys[GLFW_KEY_BACKSPACE];
        io.KeysDown[io.KeyMap[ImGuiKey_Enter]] = input_handler::Keys[GLFW_KEY_ENTER];
        io.KeysDown[io.KeyMap[ImGuiKey_Escape]] = input_handler::Keys[GLFW_KEY_ESCAPE];
        io.KeysDown[io.KeyMap[ImGuiKey_A]] = input_handler::Keys[GLFW_KEY_A];
        io.KeysDown[io.KeyMap[ImGuiKey_C]] = input_handler::Keys[GLFW_KEY_C];
        io.KeysDown[io.KeyMap[ImGuiKey_V]] = input_handler::Keys[GLFW_KEY_V];
        io.KeysDown[io.KeyMap[ImGuiKey_X]] = input_handler::Keys[GLFW_KEY_X];
        io.KeysDown[io.KeyMap[ImGuiKey_Y]] = input_handler::Keys[GLFW_KEY_Y];
        io.KeysDown[io.KeyMap[ImGuiKey_Z]] = input_handler::Keys[GLFW_KEY_Z];
        io.KeyCtrl = input_handler::Keys[GLFW_KEY_LEFT_CONTROL] || input_handler::Keys[GLFW_KEY_RIGHT_CONTROL];
        io.KeyShift = input_handler::Keys[GLFW_KEY_LEFT_SHIFT] || input_handler::Keys[GLFW_KEY_RIGHT_SHIFT];
        io.KeyAlt = input_handler::Keys[GLFW_KEY_LEFT_ALT] || input_handler::Keys[GLFW_KEY_RIGHT_ALT];
        io.KeySuper = input_handler::Keys[GLFW_KEY_LEFT_SUPER] || input_handler::Keys[GLFW_KEY_RIGHT_SUPER];

    }

	void imguiWrapper::validateBuffers() {
		
		ImDrawData* draw_data = ImGui::GetDrawData();

		VkDeviceSize vtx_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
		VkDeviceSize idx_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);

		if (vbo->InitDataSize() != vtx_size) {
			
			if (vbo) {
				vbo.reset();
			}

			vbo = std::make_unique<Buffer>(device);
			vbo->CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vtx_size);
		}

		if (ebo->InitDataSize() != idx_size) {
			
			if (ebo) {
				ebo.reset();
			}

			ebo = std::make_unique<Buffer>(device);
			ebo->CreateBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, idx_size);
		}

	}

	void imguiWrapper::updateBufferData() {
		
		const ImDrawData* draw_data = ImGui::GetDrawData();
		
		VkDeviceSize vtx_offset = 0, idx_offset = 0;
		for (int i = 0; i < draw_data->CmdListsCount; ++i) {
			const ImDrawList* list = draw_data->CmdLists[i];
			vbo->CopyToMapped(list->VtxBuffer.Data, list->VtxBuffer.Size * sizeof(ImDrawVert), vtx_offset);
			vtx_offset += list->VtxBuffer.Size * sizeof(ImDrawVert);
		}

		for (int i = 0; i < draw_data->CmdListsCount; ++i) {
			const ImDrawList* list = draw_data->CmdLists[i];
			ebo->CopyToMapped(list->IdxBuffer.Data, list->IdxBuffer.Size * sizeof(ImDrawIdx), idx_offset);
			idx_offset += list->IdxBuffer.Size * sizeof(ImDrawIdx);
		}

	}

	void imguiWrapper::updateFramegraph(const float& frame_time) {
		
		std::rotate(settings.frameTimes.begin(), settings.frameTimes.begin() + 1, settings.frameTimes.end());
		
		float frame_time_scaled = 1000.0f / (frame_time);
		
		settings.frameTimes.back() = frame_time_scaled;
		
		if (frame_time_scaled < settings.frameTimeMin) {
			settings.frameTimeMin = frame_time_scaled;
		}

		if (frame_time_scaled > settings.frameTimeMax) {
			settings.frameTimeMax = frame_time_scaled;
		}

		ImGui::PlotLines("Frame Timer", &settings.frameTimes[0], static_cast<int>(settings.frameTimes.size()), 0, "", settings.frameTimeMin, settings.frameTimeMax, ImVec2(0, 80));

		ImGui::InputFloat("Min frame time", &settings.frameTimeMin);
		ImGui::InputFloat("Max frame time", &settings.frameTimeMax);

	}

	void imguiWrapper::freeMouse(Instance * instance) {
		
		auto& io = ImGui::GetIO();
        auto* window_ptr = instance->GetWindow();

		double mouse_x, mouse_y;
		glfwGetCursorPos(window_ptr->glfwWindow(), &mouse_x, &mouse_y);
		io.MousePos = ImVec2(float(mouse_x), float(mouse_y));
        BaseScene::CameraLock = true;
		glfwSetInputMode(window_ptr->glfwWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	}

	void imguiWrapper::captureMouse(Instance* instance) {

		auto& io = ImGui::GetIO();
        auto* window_ptr = instance->GetWindow();
		io.MousePos = ImVec2(-1, -1);
        BaseScene::CameraLock = false;
		glfwSetInputMode(window_ptr->glfwWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	}

}
