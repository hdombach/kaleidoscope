#pragma once

#include <cstdint>

#include <vulkan/vulkan_core.h>

#include "util/result.hpp"
#include "util/errors.hpp"

namespace vulkan {
	/**
	 * @brief Memory backed image
	 */
	class Image {
		public:
			static util::Result<Image, KError> create(
					VkExtent2D size,
					VkFormat format,
					VkImageUsageFlags usage);

			static util::Result<Image, KError> create(
					VkExtent2D size,
					VkFormat format,
					VkImageUsageFlags usage,
					VkImageAspectFlagBits aspect,
					VkMemoryPropertyFlagBits memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			Image();

			Image(const Image& other) = delete;
			Image(Image &&other);
			Image& operator=(const Image& other) = delete;
			Image& operator=(Image &&other);

			bool empty() const;

			void destroy();
			~Image();

			VkImage image() const;
			VkImageView image_view() const;

			uint32_t width() const { return _size.width; }
			uint32_t height() const { return _size.height; }

		private:
			VkImage _image;
			VkImageView _image_view;
			VkDeviceMemory _image_memory;
			VkExtent2D _size;
	};

}
