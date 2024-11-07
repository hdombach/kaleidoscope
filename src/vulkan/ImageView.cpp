#include <vulkan/vulkan_core.h>
#include "ImageView.hpp"
#include "graphics.hpp"
/*
namespace vulkan {
	util::Result<ImageView, KError> ImageView::create(VkImage image) {
		return create_full(
				image,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_ASPECT_COLOR_BIT,
				1);
	}

	util::Result<ImageView, KError> ImageView::create_full(
			VkImage image,
			VkFormat format,
			VkImageAspectFlagBits aspect,
			uint32_t mip_levels)
	{
		auto create_info = VkImageViewCreateInfo{};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = image;
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = format;
		create_info.subresourceRange.aspectMask = aspect;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;
		create_info.subresourceRange.levelCount = mip_levels;

		VkImageView image_view;
		auto res = vkCreateImageView(
				Graphics::DEFAULT->device(),
				&create_info,
				nullptr,
				&image_view);

		if (res == VK_SUCCESS) {
			return ImageView(image_view);
		} else {
			return {res};
		}
	}

	ImageView::ImageView(ImageView &&other) {
		_image_view = other._image_view;
		other._image_view = nullptr;
	}

	ImageView& ImageView::operator=(ImageView&& other) {
		destroy();
		_image_view = other._image_view;
		other._image_view = nullptr;
		return *this;
	}

	void ImageView::destroy() {
		if (_image_view) {
			vkDestroyImageView(Graphics::DEFAULT->device(), _image_view, nullptr);
			_image_view = nullptr;
		}
	}

	ImageView::~ImageView() {
		destroy();
	}

	VkImageView& ImageView::value() {
		return _image_view;
	}

	VkImageView const& ImageView::value() const {
		return _image_view;
	}
}*/
