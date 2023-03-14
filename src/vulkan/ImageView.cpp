#include "ImageView.h"
#include "Device.h"
#include "Error.h"
#include "log.h"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	void ImageViewDeleter::operator()(ImageViewData *data) const {
		vkDestroyImageView(data->device_->raw(), data->imageView_, nullptr);
		delete data;
		util::log_memory("Deleted image view");
	}

	ImageView::ImageView(SharedDevice device, VkImage image, VkFormat format):
		base_type(new ImageViewData{nullptr})
	{
		auto createInfo = VkImageViewCreateInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		{
			auto data = get();
			auto result = vkCreateImageView(device->raw(), &createInfo, nullptr, &data->imageView_);
			if (result != VK_SUCCESS) {
				throw vulkan::Error(result);
			}
			data->device_ = device;
		}
		util::log_memory("Created image view");
	}

	VkImageView& ImageView::raw() {
		return get()->imageView_;
	}
}
