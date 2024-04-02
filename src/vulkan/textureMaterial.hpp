#pragma once

#include "material.hpp"
#include "texture.hpp"
#include "vulkan/vulkan_core.h"
#include <vector>

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

		private:
			TextureMaterialPrevImpl(PreviewRenderPass &render_pass);
			
			/* A reference */
			Texture *_texture;
			VkPipelineLayout _pipeline_layout;
			VkPipeline _pipeline;
			std::vector<VkDescriptorSet> _descriptor_sets;
			VkDescriptorSetLayout _descriptor_set_layout;

			PreviewRenderPass &_render_pass;
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
