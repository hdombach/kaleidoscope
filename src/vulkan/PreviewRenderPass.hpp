#pragma once

#include <vector>

#include <vulkan/vulkan_core.h>

#include "image.hpp"
#include "imageView.hpp"
#include "../util/result.hpp"
#include "../util/errors.hpp"

namespace vulkan {
	class PreviewRenderPass {
		public:
			static util::Result<PreviewRenderPass, KError> create(VkExtent2D size, VkRenderPass render_pass);

			PreviewRenderPass() = default;

			PreviewRenderPass(const PreviewRenderPass& other) = delete;
			PreviewRenderPass(PreviewRenderPass &&other) = default;
			PreviewRenderPass& operator=(const PreviewRenderPass& other) = delete;
			PreviewRenderPass& operator=(PreviewRenderPass &&other) = default;

			~PreviewRenderPass();

			void resize(VkExtent2D new_size);
			VkExtent2D size() const { return _size; }

			Image& depth_image() { return _depth_image; }
			Image const& depth_image() const { return _depth_image; }

			ImageView& depth_image_view() { return _depth_image_view; }
			ImageView const& depth_image_view() const { return _depth_image_view; }

			Image& color_image(int frame_index);
			Image const& color_image(int frame_index) const;

			ImageView& color_image_view(int frame_index);
			ImageView const& color_image_view(int frame_index) const;

			VkFramebuffer framebuffer(int frame_index);
			VkDescriptorSet imgui_descriptor_set(int frame_index);
		private:
			VkExtent2D _size;
			Image _depth_image;
			ImageView _depth_image_view;
			std::vector<Image> _color_images;
			std::vector<ImageView> _color_image_views;
			std::vector<VkFramebuffer> _framebuffers;
			std::vector<VkDescriptorSet> _imgui_descriptor_sets;

			VkRenderPass _render_pass; /* temporary */

			const static VkFormat _RESULT_IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;

			util::Result<void, KError> _create_images();
			void _cleanup_images();
			VkFormat _depth_format();
	};
}
