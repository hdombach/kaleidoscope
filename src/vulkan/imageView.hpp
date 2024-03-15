#pragma once

#include <vulkan/vulkan.h>
#include "../util/result.hpp"

namespace vulkan {
	class ImageView {
		public:
			static util::Result<ImageView, VkResult> create(
					VkImageViewCreateInfo info);
			static util::Result<ImageView, VkResult> create(
					VkImageViewCreateInfo info,
					VkDevice const &device);

			ImageView(): _imageView(nullptr) {}

			ImageView(const ImageView& other) = delete;
			ImageView(ImageView &&other);
			ImageView& operator=(const ImageView& other) = delete;
			ImageView& operator=(ImageView&& other);

			~ImageView();

			static VkImageViewCreateInfo create_info(VkImage &image);

			VkImageView& value();
			VkImageView const& value() const;

			VkImageView& operator*() { return value(); }
			VkImageView const& operator*() const { return value(); }

		private:
			VkImageView _imageView;

			ImageView(VkImageView imageView): _imageView(imageView) {};
	};
}
