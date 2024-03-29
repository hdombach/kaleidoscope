#include <array>

#include <vulkan/vulkan_core.h>

#include "PreviewRenderPass.hpp"
#include "defs.hpp"
#include "graphics.hpp"
#include "../util/log.hpp"
#include "imgui_impl_vulkan.h"

namespace vulkan {
	util::Result<PreviewRenderPass, KError> PreviewRenderPass::create(
			VkExtent2D size,
			VkRenderPass render_pass)
	{
		auto result = PreviewRenderPass();
		result._size = size;
		result._render_pass = render_pass;
		TRY(result._create_images());
		return result;
	}

	void PreviewRenderPass::resize(VkExtent2D new_size) {
		Graphics::DEFAULT->waitIdle();
		_cleanup_images();
		_size = new_size;
		auto res = _create_images();
		if (!res) {
			util::log_error(res.error().desc());
		}
	}

	PreviewRenderPass::~PreviewRenderPass() {
		util::log_memory("deleting preview render pass");
		_cleanup_images();
	}

	Image& PreviewRenderPass::color_image(int frame_index) {
		return _color_images[frame_index];
	}

	Image const& PreviewRenderPass::color_image(int frame_index) const {
		return _color_images[frame_index];
	}

	ImageView& PreviewRenderPass::color_image_view(int frame_index) {
		return _color_image_views[frame_index];
	}

	ImageView const& PreviewRenderPass::color_image_view(int frame_index) const {
		return _color_image_views[frame_index];
	}

	VkFramebuffer PreviewRenderPass::framebuffer(int frame_index) {
		return _framebuffers[frame_index];
	}

	VkDescriptorSet PreviewRenderPass::imgui_descriptor_set(int frame_index) {
		return _imgui_descriptor_sets[frame_index];
	}

	util::Result<void, KError> PreviewRenderPass::_create_images() {
		/* create depth resource */
		_cleanup_images();
		{
			auto image_res = Image::create(
					_size.width,
					_size.height,
					_depth_format(),
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
			TRY(image_res);
			_depth_image = std::move(image_res.value());

			auto image_view_res = _depth_image.create_image_view_full(
					_depth_format(), 
					VK_IMAGE_ASPECT_DEPTH_BIT, 
					1);
			TRY(image_view_res);
			_depth_image_view = std::move(image_view_res.value());

			Graphics::DEFAULT->transitionImageLayout(
					_depth_image.value(),
					_depth_format(),
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					1);
		}

		if (_color_images.size() != 0) {
			return KError::internal("_color_images in PreviewRenderPass must be of size 0");
		}
		if (_color_image_views.size()  != 0) {
			return KError::internal("_color_image_views in PreviewRenderPass must be of size 0");
		}
		if (_framebuffers.size() != 0) {
			return KError::internal("_framebuffers in PreviewRenderPass must be of size 0");
		}
		if (_imgui_descriptor_sets.size() != 0) {
			return KError::internal("_imgui_descriptor_sets in PreviewRenderPass must be of size 0");
		}
		for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
			auto image_res = Image::create(
					_size.width,
					_size.height,
					_RESULT_IMAGE_FORMAT,
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
						| VK_IMAGE_USAGE_STORAGE_BIT
						| VK_IMAGE_USAGE_SAMPLED_BIT);
			TRY(image_res);
			_color_images.push_back(std::move(image_res.value()));

			auto image_view_res = _color_images[i].create_image_view_full(
					_RESULT_IMAGE_FORMAT,
					VK_IMAGE_ASPECT_COLOR_BIT,
					1);
			TRY(image_view_res);
			_color_image_views.push_back(std::move(image_view_res.value()));

			Graphics::DEFAULT->transitionImageLayout(
					_color_images[i].value(),
					_RESULT_IMAGE_FORMAT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_GENERAL,
					1);

			auto attachments = std::array<VkImageView, 2>{
				_color_image_views[i].value(),
				_depth_image_view.value(),
			};

			auto framebuffer_info = VkFramebufferCreateInfo{};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.renderPass = _render_pass;
			framebuffer_info.attachmentCount = attachments.size();
			framebuffer_info.pAttachments = attachments.data();
			framebuffer_info.width = _size.width;
			framebuffer_info.height = _size.height;
			framebuffer_info.layers = 1;

			_framebuffers.push_back(nullptr);
			auto res = vkCreateFramebuffer(
					Graphics::DEFAULT->device(),
					&framebuffer_info,
					nullptr,
					&_framebuffers[i]);

			_imgui_descriptor_sets.push_back(ImGui_ImplVulkan_AddTexture(
					Graphics::DEFAULT->mainTextureSampler(),
					_color_image_views[i].value(),
					VK_IMAGE_LAYOUT_GENERAL));
		}


		return {};
	}

	void PreviewRenderPass::_cleanup_images() {
		_depth_image.~Image();
		_depth_image_view.~ImageView();
		_color_image_views.clear();
		_color_images.clear();

		for (auto framebuffer : _framebuffers) {
			vkDestroyFramebuffer(Graphics::DEFAULT->device(), framebuffer, nullptr);
		}
		_framebuffers.clear();

		for (auto descriptor_set : _imgui_descriptor_sets) {
			ImGui_ImplVulkan_RemoveTexture(descriptor_set);
		}
		_imgui_descriptor_sets.clear();
	}

	VkFormat PreviewRenderPass::_depth_format() {
		return Graphics::DEFAULT->findSupportedFormat(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
				VK_IMAGE_TILING_OPTIMAL, 
				VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
}
