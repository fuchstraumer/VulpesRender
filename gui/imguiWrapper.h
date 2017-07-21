#pragma once
#ifndef VULPES_VK_IMGUI_WRAPPER_H
#define VULPES_VK_IMGUI_WRAPPER_H
#include "stdafx.h"
#include <imgui\imgui.h>
#include "imguiShaders.h"
#include "engine\renderer\command\TransferPool.h"
#include "engine/renderer/resource/Texture.h"
#include "engine/renderer/core/LogicalDevice.h"
#include "engine/renderer\resource/Buffer.h"
#include "engine/renderer\render/GraphicsPipeline.h"
#include "engine/renderer\render/Swapchain.h"
#include "engine/renderer\core/PhysicalDevice.h"
#include "engine/renderer\core/Instance.h"
#include "engine/renderer/render/Multisampling.h"
#include "engine/renderer/resource/PipelineCache.h"

namespace vulpes {

	struct imguiSettings {
		bool displayMeshes = true;
		bool displaySkybox = true;
		std::array<float, 200> frameTimes;
		float frameTimeMin = 9999.0f, frameTimeMax = 0.0f;
	};

	struct imguiWrapper : public NonCopyable {
		
		~imguiWrapper();

		void Init(const Device* dvc, std::shared_ptr<PipelineCache> _cache, const VkRenderPass& renderpass);

		void UploadTextureData(CommandPool* transfer_pool);

		void NewFrame(Instance* instance, bool update_framegraph = true);

		void UpdateBuffers();

		void DrawFrame(VkCommandBuffer& cmd);

		imguiSettings settings;

		int imgWidth, imgHeight;

	private:

		void createResources();
		void createDescriptorPools();
		size_t loadFontTextureData();
		void uploadFontTextureData(const size_t& font_texture_size);
		void createFontTexture();
		void createDescriptorLayout();
		void createPipelineLayout();
		void allocateDescriptors();
		void updateDescriptors();
		void setupGraphicsPipelineInfo();
		void setupGraphicsPipelineCreateInfo(const VkRenderPass& renderpass);

		void validateBuffers();
		void updateBufferData();
		void updateFramegraph(const float& frame_time);
		void freeMouse(Instance* instance);
		void captureMouse(Instance* instance);

		static float mouseWheel;
		std::array<bool, 3> mouseClick;
		size_t frameIdx;
		VkCommandBuffer graphicsCmd;
		const Device* device;

		unsigned char* fontTextureData;
		std::shared_ptr<PipelineCache> cache;
		std::unique_ptr<GraphicsPipeline> pipeline;
		std::unique_ptr<Buffer> vbo, ebo;
		std::unique_ptr<Texture<gli::texture2d>> texture;
		std::unique_ptr<ShaderModule> vert, frag;

		GraphicsPipelineInfo pipelineStateInfo;
		VkGraphicsPipelineCreateInfo pipelineCreateInfo;

		VkDescriptorPool descriptorPool;
		VkDescriptorSet descriptorSet;
		VkDescriptorSetLayout descriptorSetLayout;
		VkPipelineLayout pipelineLayout;

		VkBuffer textureStaging;
		VkBufferImageCopy stagingToTextureCopy;
		VkImage fontImage;
		VkImageView fontView;
		VkSampler fontSampler;

		VkDescriptorImageInfo fontInfo;
		VkWriteDescriptorSet fontWriteSet;

	};

}
#endif // !VULPES_VK_IMGUI_WRAPPER_H
