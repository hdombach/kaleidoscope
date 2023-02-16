#include "Swapchain.h"
#include "Device.h"
#include "Error.h"
#include "ImageView.h"
#include "Surface.h"
#include "Window.h"
#include "vulkan/vulkan_core.h"
#include <memory>
#include <stdexcept>

namespace vulkan {
	void SwapchainDeleter::operator()(SwapchainData *data) const {
		vkDestroySwapchainKHR(data->device_->raw(), data->swapchain_, nullptr);
		delete data;
	}

	Swapchain::Swapchain(SharedSurface surface, SharedDevice device, SharedWindow window):
		base_type(new SwapchainData{})
	{
		auto physicalDevice = device->physicalDevice();
		auto surfaceFormat = physicalDevice.chooseSwapSurfaceFormat();
		auto presentMode = physicalDevice.chooseSwapPresentModes();
		auto extent = physicalDevice.chooseSwapExtent(window);
		auto imageCount = physicalDevice.surfaceCapabilities().minImageCount + 1;

		if (physicalDevice.surfaceCapabilities().maxImageCount > 0 && imageCount > physicalDevice.surfaceCapabilities().maxImageCount) {
			imageCount = physicalDevice.surfaceCapabilities().maxImageCount;
		}

		auto createInfo = VkSwapchainCreateInfoKHR{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.surface = surface->raw();

		auto queueFamilyIndices = std::vector<uint32_t>{physicalDevice.graphicsQueueFamily().value(), physicalDevice.presentQueueFamily().value()};

		if (physicalDevice.graphicsQueueFamily() != physicalDevice.presentQueueFamily()) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
		} else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}
		createInfo.preTransform = physicalDevice.surfaceCapabilities().currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		auto result = vkCreateSwapchainKHR(device->raw(), &createInfo, nullptr, &raw());
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}

		{
			auto data = get();
			data->device_ = device;
			data->surface_ = surface;

			uint32_t imageCount;
			vkGetSwapchainImagesKHR(device->raw(), get()->swapchain_, &imageCount, nullptr);
			data->images_.resize(imageCount);
			vkGetSwapchainImagesKHR(device->raw(), get()->swapchain_, &imageCount, get()->images_.data());

			data->imageFormat_ = createInfo.imageFormat;
			data->extent_ = createInfo.imageExtent;

			for (size_t i = 0; i < get()->images_.size(); ++i) {
				auto imageView = std::make_shared<vulkan::ImageView>(device, get()->images_[i], get()->imageFormat_);
				data->imageViews_.push_back(imageView);
			}
		}
	}

	VkSwapchainKHR& Swapchain::raw() {
		return get()->swapchain_;
	}

	SharedDevice Swapchain::device() {
		return get()->device_;
	}

	SharedSurface Swapchain::surface() {
		return get()->surface_;
	}

	VkFormat Swapchain::imageFormat() {
		return get()->imageFormat_;
	}

	VkExtent2D Swapchain::extent() {
		return get()->extent_;
	}

	std::vector<VkImage> Swapchain::images() {
		return get()->images_;
	}

	std::vector<SharedImageView> Swapchain::imageViews() {
		return get()->imageViews_;
	}
}
