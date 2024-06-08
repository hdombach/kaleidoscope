#pragma once

#include <cstdint>

#include <vulkan/vulkan_core.h>

#include "../util/result.hpp"
#include "../util/errors.hpp"
#include "ImageView.hpp"

namespace vulkan {
	/**
	 * @brief Memory backed image
	 */
	class Image {
		public:
			static util::Result<Image, KError> create(
					uint32_t width,
					uint32_t height,
					VkFormat format,
					VkImageUsageFlags usage);

			static util::Result<Image, KError> create_full(
					uint32_t width,
					uint32_t height,
					uint32_t mip_levels,
					VkFormat format,
					VkImageTiling tiling,
					VkImageUsageFlags usage,
					VkMemoryPropertyFlags properties);

			Image();

			Image(const Image& other) = delete;
			Image(Image &&other);
			Image& operator=(const Image& other) = delete;
			Image& operator=(Image &&other);

			void destroy();
			~Image();

			VkImage& value();
			VkImage const& value() const;

			VkImage& operator*() { return value(); }
			VkImage const& operator*() const { return value(); }

			util::Result<ImageView, KError> create_image_view();
			util::Result<ImageView, KError> create_image_view_full(
					VkFormat format,
					VkImageAspectFlagBits aspect,
					uint32_t mip_levels);

		private:
			VkImage _image;
			VkDeviceMemory _image_memory;
	};

}
