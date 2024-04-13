#include <vulkan/vulkan_core.h>

#include "Image.hpp"
#include "graphics.hpp"

namespace vulkan {
	util::Result<Image, KError> Image::create(
			uint32_t width,
			uint32_t height,
			VkFormat format,
			VkImageUsageFlags usage)
	{
		return create_full(
				width,
				height,
				1,
				format,
				VK_IMAGE_TILING_OPTIMAL,
				usage,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	}

	util::Result<Image, KError> Image::create_full(
			uint32_t width,
			uint32_t height,
			uint32_t mip_levels,
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkMemoryPropertyFlags properties)
	{
		auto result = Image();

		auto image_info = VkImageCreateInfo{};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.extent.width = width;
		image_info.extent.height = height;
		image_info.extent.depth = 1;
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.format = format;
		image_info.tiling = tiling;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.usage = usage;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		image_info.mipLevels = mip_levels;

		auto res = vkCreateImage(
				Graphics::DEFAULT->device(),
				&image_info,
				nullptr,
				&result._image);

		if (res != VK_SUCCESS) {
			return KError(res);
		}

		VkMemoryRequirements mem_requirements;
		vkGetImageMemoryRequirements(
				Graphics::DEFAULT->device(),
				result._image,
				&mem_requirements);

		auto memory_type = Graphics::DEFAULT->find_memory_type(
				mem_requirements.memoryTypeBits,
				properties);
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
			return KError(res);
		}

		vkBindImageMemory(
				Graphics::DEFAULT->device(),
				result._image,
				result._image_memory,
				0);

		return std::move(result);
	}

	Image::Image(): _image(nullptr), _image_memory(nullptr) {}

	Image::Image(Image &&other) {
		_image = other._image;
		other._image = nullptr;

		_image_memory = other._image_memory;
		other._image_memory = nullptr;
	}

	Image& Image::operator=(Image &&other) {
		_image = other._image;
		other._image = nullptr;

		_image_memory = other._image_memory;
		other._image_memory = nullptr;

		return *this;
	}

	Image::~Image() {
		if (_image) {
			vkDestroyImage(Graphics::DEFAULT->device(), _image, nullptr);
			_image = nullptr;
		}

		if (_image_memory) {
			vkFreeMemory(Graphics::DEFAULT->device(), _image_memory, nullptr);
			_image_memory = nullptr;
		}
	}

	VkImage& Image::value() {
		return _image;
	}

	VkImage const& Image::value() const {
		return _image;
	}

	util::Result<ImageView, KError> Image::create_image_view() {
		return ImageView::create(_image);
	}

	util::Result<ImageView, KError> Image::create_image_view_full(
			VkFormat format,
			VkImageAspectFlagBits aspect,
			uint32_t mip_levels)
	{
		return ImageView::create_full(_image, format, aspect, mip_levels);
	}
}
