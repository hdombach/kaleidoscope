#pragma once

#include "descriptorPool.hpp"
#include "mainRenderPipeline.hpp"
#include "material.hpp"
#include "shader.hpp"
#include "texture.hpp"
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
			MaterialFactory(MainRenderPipeline const &mainRenderPipeline, DescriptorPool const &descriptorPool);

			MainRenderPipeline const &mainRenderPipeline() const;
			Material *textureMaterial(Texture const *texture);
			VkDescriptorPool descriptorPool() const;
		private:
			MainRenderPipeline const &mainRenderPipeline_;
			DescriptorPool const &descriptorPool_;
	};
}
