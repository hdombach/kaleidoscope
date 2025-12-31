#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "FrameAttachment.hpp"

#include "util/result.hpp"
#include "vulkan/Error.hpp"

namespace vulkan {

	class RenderPass {
		public:
			RenderPass() = default;

			RenderPass(RenderPass const &other) = delete;
			RenderPass(RenderPass &&other);
			RenderPass &operator=(RenderPass const &other) = delete;
			RenderPass &operator=(RenderPass &&other);

			void destroy();
			~RenderPass();

			static util::Result<RenderPass, Error> create(
				std::vector<FrameAttachment> &&frame_attachments
			);

			VkRenderPass render_pass() const;

			VkFramebuffer framebuffer() const;

			std::vector<FrameAttachment> const &frame_attachments() const;

		private:
			std::vector<FrameAttachment> _frame_attachments;
			VkRenderPass _render_pass = nullptr;
			VkFramebuffer _framebuffer = nullptr;
	};
}
