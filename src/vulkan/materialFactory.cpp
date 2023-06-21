#include "materialFactory.h"
#include "descriptorPool.h"
#include "mainRenderPipeline.h"
#include "textureMaterial.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	MaterialFactory::MaterialFactory(MainRenderPipeline const &mainRenderPipeline, DescriptorPool const &descriptorPool):
		mainRenderPipeline_(mainRenderPipeline),
		descriptorPool_(descriptorPool)
	{}

	MainRenderPipeline const &MaterialFactory::mainRenderPipeline() const {
		return mainRenderPipeline_;
	}

	Material *MaterialFactory::textureMaterial(Texture const *texture) {
		return new TextureMaterial(*this, texture);
	}

	VkDescriptorPool MaterialFactory::descriptorPool() const {
		return descriptorPool_.descriptorPool();
	}
}
