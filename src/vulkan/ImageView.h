#pragma once

#include "Device.h"
#include "vulkan/vulkan_core.h"
#include <memory>

namespace vulkan {
	class ImageView;
	using SharedImageView = std::shared_ptr<ImageView>;
	using UniqueImageView = std::unique_ptr<ImageView>;

	class ImageView {
		public:
			static SharedImageView createShared(VkImageViewCreateInfo &createInfo, SharedDevice device);
			static UniqueImageView createUnique(VkImageViewCreateInfo &createInfo, SharedDevice device);
			VkImageView& operator*();
			VkImageView& raw();
			~ImageView();

		private:
			ImageView(VkImageViewCreateInfo &createInfo, SharedDevice device);

			VkImageView imageView_;
			SharedDevice device_;
	};

	class ImageViewFactory {
		public:
			ImageViewFactory(VkImage image, SharedDevice device, VkFormat format);

			ImageViewFactory &defaultConfig();
			SharedImageView createShared();
			UniqueImageView createUnique();

		private:
			VkImageViewCreateInfo createInfo_{};

			VkImage image_;
			SharedDevice device_;
			VkFormat format_;
	};
}
