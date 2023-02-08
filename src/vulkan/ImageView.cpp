#include "ImageView.h"
#include "Device.h"
#include "Error.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	SharedImageView ImageView::createShared(VkImageViewCreateInfo &createInfo, SharedDevice device) {
		return SharedImageView(new ImageView(createInfo, device));
	}

	UniqueImageView ImageView::createUnique(VkImageViewCreateInfo &createInfo, SharedDevice device) {
		return UniqueImageView(new ImageView(createInfo, device));
	}

	VkImageView& ImageView::operator*() {
		return imageView_;
	}

	VkImageView& ImageView::raw() {
		return imageView_;
	}

	ImageView::~ImageView() {
		vkDestroyImageView(**device_, imageView_, nullptr);
	}

	ImageView::ImageView(VkImageViewCreateInfo &createInfo, SharedDevice device): device_(device) {
		auto result = vkCreateImageView(**device, &createInfo, nullptr, &imageView_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}
	}

	/**** factory ****/

	ImageViewFactory::ImageViewFactory(
			VkImage image,
			SharedDevice device,
			VkFormat format): device_(device), format_(format), image_(image) {
		createInfo_.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	}

	ImageViewFactory &ImageViewFactory::defaultConfig() {
		createInfo_.image = image_;
		createInfo_.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo_.format = format_;
		createInfo_.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo_.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo_.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo_.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo_.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo_.subresourceRange.baseMipLevel = 0;
		createInfo_.subresourceRange.levelCount = 1;
		createInfo_.subresourceRange.baseArrayLayer = 0;
		createInfo_.subresourceRange.layerCount = 1;

		return *this;
	}

	SharedImageView ImageViewFactory::createShared() {
		return ImageView::createShared(createInfo_, device_);
	}

	UniqueImageView ImageViewFactory::createUnique() {
		return ImageView::createUnique(createInfo_, device_);
	}
}
