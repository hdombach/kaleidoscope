#pragma once

#include "Device.h"
#include "vulkan/vulkan_core.h"
#include <memory>

namespace vulkan {
	class ImageView;
	using SharedImageView = std::shared_ptr<ImageView>;

	struct ImageViewData {
		VkImageView imageView_;
		SharedDevice device_;
	};
	struct ImageViewDeleter {
		void operator()(ImageViewData *data) const;
	};

	class ImageView: public std::unique_ptr<ImageViewData, ImageViewDeleter> {
		public:
			using base_type = std::unique_ptr<ImageViewData, ImageViewDeleter>;

			ImageView(SharedDevice device, VkImage image, VkFormat format);
			VkImageView& raw();
	};
}
