#pragma once

#include <glm/fwd.hpp>
#include <vulkan/vulkan.h>

#include "../util/result.hpp"
#include "../util/errors.hpp"
#include "Shader.hpp"

namespace vulkan {
	class MaterialPreviewImpl;
	class PreviewRenderPass;

	class Material {
		public:
			using PreviewImpl = MaterialPreviewImpl;

			virtual ~Material() = default;

			/** @brief Adds the preview renderpass specific implimentation */
			virtual util::Result<void, KError> add_preview(
					PreviewRenderPass &preview_render_pass) = 0;

			virtual MaterialPreviewImpl *preview_impl() = 0;
			virtual MaterialPreviewImpl const *preview_impl() const = 0;

	};

	class MaterialPreviewImpl {
		public:
			virtual ~MaterialPreviewImpl() = default;
			virtual VkPipelineLayout pipeline_layout() = 0;
			virtual VkPipeline pipeline() = 0;
			virtual VkDescriptorSet get_descriptor_set(uint32_t frame_index) = 0;
			virtual void update_uniform(uint32_t frame_index, glm::vec3 position, glm::vec2 viewport_size) = 0;

		protected:

			/**
			 * @param[in] vertex_shader
			 * @param[in] fragment_shader
			 * @param[in] descriptor_set_layout
			 * @param[out] pipeline
			 * @param[out] pipeline_layout
			 */
			static util::Result<void, KError> _create_pipeline(
					Shader &vertex_shader,
					Shader &fragment_shader,
					PreviewRenderPass &render_pass,
					VkDescriptorSetLayout *descriptor_set_layout,
					VkPipeline *pipeline,
					VkPipelineLayout *pipeline_layout);

	};
}