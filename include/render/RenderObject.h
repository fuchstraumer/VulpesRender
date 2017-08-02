#ifndef VULPES_RENDER_OBJECT_H
#define VULPES_RENDER_OBJECT_H
#include "vpr_stdafx.h"
#include "../ForwardDecl.h"

#include "resource/DescriptorPool.h"
#include "resource/DescriptorSet.h"
#include "resource/PipelineLayout.h"
#include "resource/PipelineCache.h"
#include "render/GraphicsPipeline.h"

namespace vulpes {

	class RenderObject {
		RenderObject(const RenderObject&) = delete;
		RenderObject& operator=(const RenderObject&) = delete;
	public:



	private:

		virtual void createDescriptors();
		virtual void createShaders() = 0;
		virtual void createPipelineLayout() = 0;
		virtual void setupPipelineInfo() = 0;
		virtual void setupGraphicsPipeline() = 0;

		const Device* device;

		// Access to this object shared among all descriptor sets.
		std::shared_ptr<DescriptorPool> descriptorPool;
		std::unique_ptr<DescriptorSet> descriptorSet;

		std::vector<VkPushConstantRange> pushConstants;

		std::unique_ptr<GraphicsPipeline> pipeline;
		std::unique_ptr<PipelineLayout> pipelineLayout;
		std::unique_ptr<PipelineCache> pipelineCache;
	};

}

#endif // !VULPES_RENDER_OBJECT_H
