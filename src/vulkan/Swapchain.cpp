#include "Swapchain.h"
#include "Device.h"
#include "Error.h"
#include "ImageView.h"
#include "Surface.h"
#include "Window.h"
#include "vulkan/vulkan_core.h"
#include <stdexcept>

namespace vulkan {
	SharedSwapchain Swapchain::createShared(
			VkSwapchainCreateInfoKHR &createInfo,
			SharedDevice device,
			SharedSurface surface,
			SharedWindow window)
	{
		return SharedSwapchain(new Swapchain(createInfo, device, surface, window));
	}

	UniqueSwapchain Swapchain::createUnique(
			VkSwapchainCreateInfoKHR &createInfo,
			SharedDevice device,
			SharedSurface surface,
			SharedWindow window)
	{
		return UniqueSwapchain(new Swapchain(createInfo, device, surface, window));
	}

	VkSwapchainKHR& Swapchain::operator*() {
		return swapchain_;
	}

	Swapchain::~Swapchain() {
		vkDestroySwapchainKHR(**device_, swapchain_, nullptr);
	}

	SharedDevice Swapchain::device() {
		return device_;
	}

	SharedSurface Swapchain::surface() {
		return surface_;
	}

	VkFormat Swapchain::imageFormat() {
		return imageFormat_;
	}

	VkExtent2D Swapchain::extent() {
		return extent_;
	}

	std::vector<VkImage> Swapchain::images() {
		return images_;
	}

	std::vector<SharedImageView> Swapchain::imageViews() {
		return imageViews_;
	}

	Swapchain::Swapchain(
			VkSwapchainCreateInfoKHR &createInfo,
			SharedDevice device,
			SharedSurface surface,
			SharedWindow window): device_(device)
	{
		auto result = vkCreateSwapchainKHR(**device, &createInfo, nullptr, &swapchain_);
		if (result != VK_SUCCESS) {
			throw vulkan::Error(result);
		}

		uint32_t imageCount;
		vkGetSwapchainImagesKHR(**device, swapchain_, &imageCount, nullptr);
		images_.resize(imageCount);
		vkGetSwapchainImagesKHR(**device, swapchain_, &imageCount, images_.data());

		imageFormat_ = createInfo.imageFormat;
		extent_ = createInfo.imageExtent;
		
		for (size_t i = 0; i < images_.size(); i++) {
			auto imageView = ImageViewFactory(images_[i], device, imageFormat_).defaultConfig().createShared();
			imageViews_.push_back(imageView);
		}
	}

	SwapchainFactory::SwapchainFactory(
			SharedSurface surface,
			SharedDevice device,
			SharedWindow window): device_(device), surface_(surface), window_(window) {
		createInfo_.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	}

	SwapchainFactory &SwapchainFactory::defaultConfig() {
		auto physicalDevice = device_->physicalDevice();
		auto surfaceFormat = physicalDevice.chooseSwapSurfaceFormat();
		auto presentMode = physicalDevice.chooseSwapPresentModes();
		auto extent = physicalDevice.chooseSwapExtent(window_);
		auto imageCount = physicalDevice.surfaceCapabilities().minImageCount + 1;

		if (physicalDevice.surfaceCapabilities().maxImageCount > 0 && imageCount > physicalDevice.surfaceCapabilities().maxImageCount) {
			imageCount = physicalDevice.surfaceCapabilities().maxImageCount;
		}

		createInfo_.surface = **surface_;
		createInfo_.minImageCount = imageCount;
		createInfo_.imageFormat = surfaceFormat.format;
		createInfo_.imageColorSpace = surfaceFormat.colorSpace;
		createInfo_.imageExtent = extent;
		createInfo_.imageArrayLayers = 1;
		createInfo_.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


		queueFamilyIndices_ = {physicalDevice.graphicsQueueFamily().value(), physicalDevice.presentQueueFamily().value()};

		if (physicalDevice.graphicsQueueFamily() != physicalDevice.presentQueueFamily()) {
			createInfo_.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo_.queueFamilyIndexCount = 2;
			createInfo_.pQueueFamilyIndices = queueFamilyIndices_.data();
		} else {
			createInfo_.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo_.queueFamilyIndexCount = 0;
			createInfo_.pQueueFamilyIndices = nullptr;
		}
		createInfo_.preTransform = physicalDevice.surfaceCapabilities().currentTransform;
		createInfo_.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo_.presentMode = presentMode;
		createInfo_.clipped = VK_TRUE;
		createInfo_.oldSwapchain = VK_NULL_HANDLE;

		return *this;
	}

	SharedSwapchain SwapchainFactory::createShared() {
		return Swapchain::createShared(createInfo_, device_, surface_, window_);
	}

	UniqueSwapchain SwapchainFactory::createUnique() {
		return Swapchain::createUnique(createInfo_, device_, surface_, window_);
	}
}
