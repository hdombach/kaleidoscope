#include "materialFactory.h"
#include "mainRenderPipeline.h"
#include "textureMaterial.h"

namespace vulkan {
	MaterialFactory::MaterialFactory(MainRenderPipeline const &mainRenderPipeline):
		mainRenderPipeline_(mainRenderPipeline)
	{}

	MainRenderPipeline const &MaterialFactory::mainRenderPipeline() const {
		return mainRenderPipeline_;
	}

	Material *MaterialFactory::textureMaterial(Texture const *texture) {
		return new TextureMaterial(*this, texture);
	}
}
