#include <vulkan/vulkan_core.h>

#include "Image.hpp"
#include "graphics.hpp"

namespace vulkan {

	util::Result<Image, Error> Image::create(
			VkExtent2D size,
			VkFormat format,
			VkImageUsageFlags usage)
	{
		return create(
				size,
				format,
				usage,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	util::Result<Image, Error> Image::create(
			VkExtent2D size,
			VkFormat format,
			VkImageUsageFlags usage,
			VkImageAspectFlagBits aspect,
			VkMemoryPropertyFlagBits memory_properties)
	{
		auto result = Image();

		/* Create Image */
		auto image_info = VkImageCreateInfo{};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.extent.width = size.width;
		image_info.extent.height = size.height;
		image_info.extent.depth = 1;
		image_info.arrayLayers = 1;
		image_info.format = format;
		image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.usage = usage;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_info.mipLevels = 1;

		auto res = vkCreateImage(
				Graphics::DEFAULT->device(),
				&image_info,
				nullptr,
				&result._image);

		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create image. ", VkError(res));
		}

		auto mem_requirements = VkMemoryRequirements{};
		vkGetImageMemoryRequirements(
				Graphics::DEFAULT->device(),
				result._image,
				&mem_requirements);

		auto memory_type = Graphics::DEFAULT->find_memory_type(
				mem_requirements.memoryTypeBits, 
				memory_properties);
		TRY(memory_type);

		auto alloc_info_res = VkMemoryAllocateInfo{};
		alloc_info_res.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info_res.allocationSize = mem_requirements.size;
		alloc_info_res.memoryTypeIndex = memory_type.value();

		res = vkAllocateMemory(
				Graphics::DEFAULT->device(), 
				&alloc_info_res, 
				nullptr, 
				&result._image_memory);
		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not allocate memory.", VkError(res));
		}

		vkBindImageMemory(
				Graphics::DEFAULT->device(),
				result._image,
				result._image_memory,
				0);

		/* Image View */
		auto image_view_info = VkImageViewCreateInfo{};
		image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_info.image = result._image;
		image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_info.format = format;
		image_view_info.subresourceRange.aspectMask = aspect;
		image_view_info.subresourceRange.baseMipLevel = 0;
		image_view_info.subresourceRange.levelCount = 1;
		image_view_info.subresourceRange.baseArrayLayer = 0;
		image_view_info.subresourceRange.layerCount = 1;
		image_view_info.subresourceRange.levelCount = 1;

		res = vkCreateImageView(
				Graphics::DEFAULT->device(),
				&image_view_info,
				nullptr,
				&result._image_view);
		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create image view.", VkError(res));
		}

		result._size = size;

		return std::move(result);
	}

	Image::Image():
		_image(nullptr),
		_image_view(nullptr),
		_image_memory(nullptr)
	{}

	Image::Image(Image &&other) {
		_image = other._image;
		other._image = nullptr;

		_image_view = other._image_view;
		other._image_view = nullptr;

		_image_memory = other._image_memory;
		other._image_memory = nullptr;

		_size = other._size;
	}

	Image& Image::operator=(Image &&other) {
		destroy();
		_image = other._image;
		other._image = nullptr;

		_image_view = other._image_view;
		other._image_view = nullptr;

		_image_memory = other._image_memory;
		other._image_memory = nullptr;

		_size = other._size;

		return *this;
	}

	void Image::destroy() {
		if (_image) {
			vkDestroyImage(Graphics::DEFAULT->device(), _image, nullptr);
			_image = nullptr;
		}

		if (_image_view) {
			vkDestroyImageView(Graphics::DEFAULT->device(), _image_view, nullptr);
			_image_view = nullptr;
		}

		if (_image_memory) {
			vkFreeMemory(Graphics::DEFAULT->device(), _image_memory, nullptr);
			_image_memory = nullptr;
		}
	}

	Image::~Image() {
		destroy();
	}

	VkImage Image::image() const {
		return _image;
	}

	VkImageView Image::image_view() const {
		return _image_view;
	}
}
