#pragma once

#include "mainRenderPipeline.h"
#include "material.h"
#include "shader.h"
#include "texture.h"
#include <string>
#include <unordered_map>

namespace vulkan {
	/*
	 * The idea behind having an entire factory is that
	 * materials will need to build pipelines for several
	 * different render passes eventually
	 * Also, in the future, common descriptor sets should
	 * be created in order to be shared between materials
	 */
	class MaterialFactory {
		public:
			MaterialFactory(MainRenderPipeline const &mainRenderPipeline);

			MainRenderPipeline const &mainRenderPipeline() const;
			Material *textureMaterial(Texture const *texture);
		private:
			MainRenderPipeline const &mainRenderPipeline_;
	};
}
