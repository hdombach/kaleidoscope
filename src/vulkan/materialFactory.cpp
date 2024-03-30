#include "materialFactory.hpp"
#include "descriptorPool.hpp"
#include "PreviewRenderPass.hpp"
#include "textureMaterial.hpp"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	MaterialFactory::MaterialFactory(PreviewRenderPass &mainRenderPipeline, DescriptorPool const &descriptorPool):
		mainRenderPipeline_(mainRenderPipeline),
		descriptorPool_(descriptorPool)
	{}

	PreviewRenderPass &MaterialFactory::mainRenderPipeline() {
		return mainRenderPipeline_;
	}

	Material *MaterialFactory::textureMaterial(Texture *texture) {
		return new TextureMaterial(*this, texture);
	}

	VkDescriptorPool MaterialFactory::descriptorPool() const {
		return descriptorPool_.descriptorPool();
	}
}
