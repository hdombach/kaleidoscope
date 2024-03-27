#pragma once

#include <vulkan/vulkan_core.h>

#include "image.hpp"
#include "imageView.hpp"
#include "../util/result.hpp"
#include "../util/errors.hpp"

namespace vulkan {
	class PreviewRenderPass {
		public:
			static util::Result<PreviewRenderPass, KError> create(VkExtent2D size);

			PreviewRenderPass() = default;

			PreviewRenderPass(const PreviewRenderPass& other) = delete;
			PreviewRenderPass(PreviewRenderPass &&other) = default;
			PreviewRenderPass& operator=(const PreviewRenderPass& other) = delete;
			PreviewRenderPass& operator=(PreviewRenderPass &&other) = default;

			~PreviewRenderPass() = default;

			void resize(VkExtent2D new_size);
			VkExtent2D size() const { return _size; }

			Image& depth_image() { return _depth_image; }
			Image const& depth_image() const { return _depth_image; }

			ImageView& depth_image_view() { return _depth_image_view; }
			ImageView const& depth_image_view() const { return _depth_image_view; }

		private:
			VkExtent2D _size;
			Image _depth_image;
			ImageView _depth_image_view;

			util::Result<void, KError> _create_images();
			void _cleanup_images();
			VkFormat _depth_format();
	};
}
