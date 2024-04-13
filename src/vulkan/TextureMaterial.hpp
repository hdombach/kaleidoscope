#pragma once

#include <vector>

#include <vulkan/vulkan_core.h>

#include "DescriptorSet.hpp"
#include "Material.hpp"
#include "Texture.hpp"
#include "UniformBufferObject.hpp"

namespace vulkan {
	class TextureMaterialPrevImpl: public MaterialPreviewImpl {
		public:
			static util::Result<TextureMaterialPrevImpl, KError> create(
					PreviewRenderPass &render_pass,
					Texture *texture);

			TextureMaterialPrevImpl(const TextureMaterialPrevImpl& other) = delete;
			TextureMaterialPrevImpl(TextureMaterialPrevImpl &&other);
			TextureMaterialPrevImpl& operator=(const TextureMaterialPrevImpl& other) = delete;
			TextureMaterialPrevImpl& operator=(TextureMaterialPrevImpl&& other);

			~TextureMaterialPrevImpl() override;

			VkPipelineLayout pipeline_layout() override;
			VkPipeline pipeline() override;
			VkDescriptorSet get_descriptor_set(uint32_t frame_index) override;
			void update_uniform(uint32_t frame_index, glm::vec3 position, glm::vec2 viewport_size) override;

		private:
			TextureMaterialPrevImpl(DescriptorPool &descriptor_pool);
			
			/* A reference */
			Texture *_texture;
			VkPipelineLayout _pipeline_layout;
			VkPipeline _pipeline;

			DescriptorSets _descriptor_sets;
			std::vector<MappedUniformObject> _mapped_uniforms;
	};


	class TextureMaterial: public Material {
		public:
			using PreviewImpl = TextureMaterialPrevImpl;

			TextureMaterial(Texture* texture);

			~TextureMaterial() override = default;

			TextureMaterial(const TextureMaterial& other) = delete;
			TextureMaterial(TextureMaterial &&other) = default;
			TextureMaterial& operator=(const TextureMaterial& other) = delete;
			TextureMaterial& operator=(TextureMaterial&& other) = default;

			util::Result<void, KError> add_preview(
					PreviewRenderPass &preview_render_pass) override;

			MaterialPreviewImpl *preview_impl() override;
			MaterialPreviewImpl const *preview_impl() const override;

		private:
			Texture *_texture;

			std::optional<TextureMaterialPrevImpl> _preview_impl;
	};

}
