#pragma once

#include <optional>
#include <vector>

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

#include "DescriptorSet.hpp"
#include "Material.hpp"
#include "PreviewRenderPass.hpp"
#include "MappedUniform.hpp"

namespace vulkan {
	class ColorMaterialPrevImpl: public MaterialPreviewImpl {
		public:
			struct UniformBuffer {
				alignas(16) glm::mat4 object_transformation;
				alignas(16) glm::vec3 color;
			};

			using MappedUniform = MappedUniform<UniformBuffer>;

			static util::Result<ColorMaterialPrevImpl, KError> create(
					PreviewRenderPass &render_pass,
					glm::vec3 color);

			ColorMaterialPrevImpl(const ColorMaterialPrevImpl& other) = delete;
			ColorMaterialPrevImpl(ColorMaterialPrevImpl &&other);
			ColorMaterialPrevImpl& operator=(const ColorMaterialPrevImpl& other) = delete;
			ColorMaterialPrevImpl& operator=(ColorMaterialPrevImpl&& other);
			ColorMaterialPrevImpl() = default;

			~ColorMaterialPrevImpl() override;

			VkPipelineLayout pipeline_layout() override;
			VkPipeline pipeline() override;
			VkDescriptorSet get_descriptor_set(uint32_t frame_index) override;
			void update_uniform(
					uint32_t frame_index,
					glm::vec3 position,
					glm::vec2 viewport_size) override;

		private:
			VkPipelineLayout _pipeline_layout;
			VkPipeline _pipeline;
			DescriptorSets _descriptor_sets;
			std::vector<MappedUniform> _mapped_uniforms;
			glm::vec3 _color;
	};

	class ColorMaterial: public Material {
		public:
			struct UniformBuffer {
				alignas(16) glm::mat4 object_transformation;
				alignas(16) glm::vec3 color;
			};

			using PreviewImpl = ColorMaterialPrevImpl;

			ColorMaterial(uint32_t id, glm::vec3 color);

			~ColorMaterial() override = default;

			std::vector<types::ShaderResource> const &resources() const override;
			uint32_t id() const override;

			ColorMaterial(const ColorMaterial& other) = delete;
			ColorMaterial(ColorMaterial &&other) = default;
			ColorMaterial& operator=(const ColorMaterial& other) = delete;
			ColorMaterial& operator=(ColorMaterial&& other) = default;

			util::Result<void, KError> add_preview(
					PreviewRenderPass &preview_render_pass) override;

			ColorMaterialPrevImpl *preview_impl() override;
			ColorMaterialPrevImpl const *preview_impl() const override;

		private:
			uint32_t _id;

			glm::vec3 _color;

			std::vector<types::ShaderResource> _resources;

			VType<UniformBuffer> _uniform;

			std::optional<ColorMaterialPrevImpl> _preview_impl;
	};
}
