#pragma once

#include <vector>

#include <vulkan/vulkan_core.h>

#include "DescriptorSet.hpp"
#include "MappedUniform.hpp"
#include "Material.hpp"
#include "Texture.hpp"
#include "VType.hpp"

namespace vulkan {
	class TextureMaterialPrevImpl: public MaterialPreviewImpl {
		public:
			struct UniformBuffer {
				alignas(16) glm::mat4 object_transformation;
			} __attribute__((packed));
			using MappedUniform = MappedUniform<UniformBuffer>;

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
			std::vector<MappedUniform> _mapped_uniforms;
	};


	class TextureMaterial: public Material {
		public:
			struct UniformBuffer {
				alignas(16) glm::mat4 object_transformation;
			};

			using PreviewImpl = TextureMaterialPrevImpl;

			TextureMaterial(uint32_t id, Texture* texture);

			~TextureMaterial() override = default;

			TextureMaterial(const TextureMaterial& other) = delete;
			TextureMaterial(TextureMaterial &&other) = default;
			TextureMaterial& operator=(const TextureMaterial& other) = delete;
			TextureMaterial& operator=(TextureMaterial&& other) = default;

			std::vector<types::ShaderResource> const &resources() const override { return _resources; }
			uint32_t id() const override { return _id; }

			util::Result<void, KError> add_preview(
					PreviewRenderPass &preview_render_pass) override;

			MaterialPreviewImpl *preview_impl() override;
			MaterialPreviewImpl const *preview_impl() const override;

		private:
			Texture *_texture;
			uint32_t _id;

			std::vector<types::ShaderResource> _resources;

			VType<UniformBuffer> _uniform;

			std::optional<TextureMaterialPrevImpl> _preview_impl;
	};

}
