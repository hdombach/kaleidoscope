#pragma once

#include "descriptorPool.hpp"
#include "PreviewRenderPass.hpp"
#include "material.hpp"
#include "texture.hpp"

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
			MaterialFactory(PreviewRenderPass &mainRenderPipeline, DescriptorPool const &descriptorPool);

			PreviewRenderPass &mainRenderPipeline();
			Material *textureMaterial(Texture *texture);
			VkDescriptorPool descriptorPool() const;
		private:
			PreviewRenderPass &mainRenderPipeline_;
			DescriptorPool const &descriptorPool_;
	};
}
