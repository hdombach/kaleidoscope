#include <vulkan/vulkan_core.h>
#include "imageView.hpp"
#include "graphics.hpp"

namespace vulkan {
	util::Result<ImageView, VkResult> ImageView::create(VkImageViewCreateInfo info) {
		return ImageView::create(info, Graphics::DEFAULT->device());
	}

	util::Result<ImageView, VkResult> ImageView::create(
			VkImageViewCreateInfo info,
			VkDevice const &device)
	{
		VkImageView imageView;
		
		auto res = vkCreateImageView(
				device,
				&info,
				nullptr,
				&imageView);

		if (res == VK_SUCCESS) {
			return ImageView(imageView);
		} else {
			return res;
		}
	}

	ImageView::ImageView(ImageView &&other) {
		_imageView = other._imageView;
		other._imageView = nullptr;
	}

	ImageView& ImageView::operator=(ImageView&& other) {
		_imageView = other._imageView;
		other._imageView = nullptr;
		return *this;
	}

	ImageView::~ImageView() {
		if (_imageView) {
			vkDestroyImageView(Graphics::DEFAULT->device(), _imageView, nullptr);
			_imageView = nullptr;
		}
	}

	VkImageViewCreateInfo ImageView::create_info(VkImage &image) {
		auto result = VkImageViewCreateInfo{};
		result.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		result.image = image;
		result.viewType = VK_IMAGE_VIEW_TYPE_2D;
		result.format = VK_FORMAT_R8G8B8A8_SRGB;
		result.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		result.subresourceRange.baseMipLevel = 0;
		result.subresourceRange.levelCount = 1;
		result.subresourceRange.baseArrayLayer = 0;
		result.subresourceRange.layerCount = 1;
		result.subresourceRange.levelCount = 1;

		return result;
	}

	VkImageView& ImageView::value() {
		return _imageView;
	}

	VkImageView const& ImageView::value() const {
		return _imageView;
	}
}
