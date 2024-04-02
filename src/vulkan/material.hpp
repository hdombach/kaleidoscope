#pragma once

#include <vulkan/vulkan.h>

#include "../util/result.hpp"
#include "../util/errors.hpp"

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
	};
}
