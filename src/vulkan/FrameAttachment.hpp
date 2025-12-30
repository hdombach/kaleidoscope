#pragma once

#include <vulkan/vulkan_core.h>
#include "Error.hpp"
#include "util/result.hpp"

namespace vulkan {
	class Image;

	/**
	 * @brief Attachments to be used in the framebuffer of a pipeline
	 */
	class FrameAttachment {
		public:
			FrameAttachment() = default;

			static FrameAttachment create(Image const &image);

			/**
			 * @brief Assigns a clear value
			 */
			FrameAttachment &set_clear_value(VkClearValue const &clear_value);

			/**
			 * @brief Resolves the VkAttachmentDescription for the render pass
			 */
			util::Result<VkAttachmentDescription, Error> attachment_description();

			/**
			 * @brief Resolves the VkPipelineColorBlendAttachmentState for the render pass
			 */
			util::Result<VkPipelineColorBlendAttachmentState, Error> blend_attachment_state();

			/**
			 * @brief Gets the clear color that will be used by the render pass
			 */
			VkClearValue clear_color() const;

		private:
			Image const *_image=nullptr;

			VkClearValue _clear_color;
	};
}
