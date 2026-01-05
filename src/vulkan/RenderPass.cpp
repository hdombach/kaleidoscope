#include <vulkan/vulkan_core.h>

#include "RenderPass.hpp"
#include "util/Util.hpp"
#include "graphics.hpp"

namespace vulkan {
	RenderPass::RenderPass(RenderPass &&other) {
		_frame_attachments = std::move(other._frame_attachments);
		_render_pass = util::move_ptr(other._render_pass);
		_framebuffer = util::move_ptr(other._framebuffer);
	}

	RenderPass &RenderPass::operator=(RenderPass &&other) {
		_frame_attachments = std::move(other._frame_attachments);
		_render_pass = util::move_ptr(other._render_pass);
		_framebuffer = util::move_ptr(other._framebuffer);

		return *this;
	}

	void RenderPass::destroy() {
		if (_render_pass) {
			vkDestroyRenderPass(Graphics::DEFAULT->device(), _render_pass, nullptr);
			_render_pass = nullptr;
		}
		if (_framebuffer) {
			vkDestroyFramebuffer(Graphics::DEFAULT->device(), _framebuffer, nullptr);
			_framebuffer = nullptr;
		}
	}

	RenderPass::~RenderPass() { destroy(); }

	util::Result<RenderPass, Error> RenderPass::create(
		std::vector<FrameAttachment> &&frame_attachments
	) {
		auto render_pass = RenderPass();
		render_pass._frame_attachments = std::move(frame_attachments);

		if (render_pass._frame_attachments.empty()) {
			return Error(ErrorType::INVALID_ARG, "Cannot create render pass with 0 frame attachments");
		}

		auto descriptions = std::vector<VkAttachmentDescription>();
		auto color_attachment_refs = std::vector<VkAttachmentReference>();
		VkAttachmentReference *depth_attachment_ref = nullptr;
		VkAttachmentReference depth_attachment_ref_value;

		int i = 0;
		for (auto &attachment : render_pass._frame_attachments) {
			VkAttachmentDescription description;

			if (auto err = attachment.attachment_description().move_or(description)) {
				return Error(
					ErrorType::SHADER_RESOURCE,
					util::f("Could not resolve attachment description for framebuffer ", i),
					err.value()
				);
			}

			descriptions.push_back(description);

			auto attachment_ref = VkAttachmentReference{};
			attachment_ref.attachment = i;
			attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			if (attachment.depth()) {
				if (depth_attachment_ref != nullptr) {
					return Error(ErrorType::SHADER_RESOURCE, "Cannot attach more than one depth buffer");
				}
				depth_attachment_ref_value = attachment_ref;
				depth_attachment_ref = &depth_attachment_ref_value;
				depth_attachment_ref->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			} else {
				color_attachment_refs.push_back(attachment_ref);
			}

			i++;
		}

		auto subpass = VkSubpassDescription{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = color_attachment_refs.size();
		subpass.pColorAttachments = color_attachment_refs.data();
		subpass.pDepthStencilAttachment = depth_attachment_ref;

		auto render_pass_info = VkRenderPassCreateInfo{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(descriptions.size());
		render_pass_info.pAttachments = descriptions.data();
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;

		auto dependency = VkSubpassDependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT |
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

		auto res = vkCreateRenderPass(
			Graphics::DEFAULT->device(),
			&render_pass_info,
			nullptr,
			&render_pass._render_pass
		);

		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create render pass", VkError(res));
		}

		// Create framebuffer
		auto frame_images = std::vector<VkImageView>();
		for (auto &attachment : render_pass._frame_attachments) {
			frame_images.push_back(attachment.image_view());
		}

		auto framebuffer_info = VkFramebufferCreateInfo{};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = render_pass._render_pass;
		framebuffer_info.attachmentCount = frame_images.size();
		framebuffer_info.pAttachments = frame_images.data();
		framebuffer_info.width = render_pass._frame_attachments.front().size().width;
		framebuffer_info.height = render_pass._frame_attachments.front().size().height;
		framebuffer_info.layers = 1;

		res = vkCreateFramebuffer(
			Graphics::DEFAULT->device(), 
			&framebuffer_info, 
			nullptr, 
			&render_pass._framebuffer
		);

		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create framebuffer", VkError(res));
		}

		return render_pass;
	}

	bool RenderPass::has_value() const {
		return _render_pass != nullptr;
	}

	VkRenderPass RenderPass::render_pass() const {
		return _render_pass;
	}

	VkFramebuffer RenderPass::framebuffer() const {
		return _framebuffer;
	}

	std::vector<FrameAttachment> const &RenderPass::frame_attachments() const {
		return _frame_attachments;
	}

	std::vector<VkClearValue> RenderPass::clear_values() const {
		auto r = std::vector<VkClearValue>();

		for (auto &attachment : _frame_attachments) {
			r.push_back(attachment.clear_color());
		}

		return r;
	}
}
