#pragma once

#include "../util/result.hpp"
#include "../util/errors.hpp"
/*
namespace vulkan {
	class ImageView {
		public:
			static util::Result<ImageView, KError> create(VkImage image);
			static util::Result<ImageView, KError> create_full(
					VkImage image,
					VkFormat format,
					VkImageAspectFlagBits aspect,
					uint32_t mip_levels);

			ImageView(): _image_view(nullptr) {}

			ImageView(const ImageView& other) = delete;
			ImageView(ImageView &&other);
			ImageView& operator=(const ImageView& other) = delete;
			ImageView& operator=(ImageView&& other);

			void destroy();
			~ImageView();

			VkImageView& value();
			VkImageView const& value() const;

			VkImageView& operator*() { return value(); }
			VkImageView const& operator*() const { return value(); }

		private:
			ImageView(VkImageView imageView): _image_view(imageView) {};

		private:
			VkImageView _image_view;
	};
}*/
