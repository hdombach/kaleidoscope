#pragma once
#include <vulkan/vulkan.h>
#include "errors.hpp"
#include "texture.hpp"
#include "result.hpp"
#include "vulkan/vulkan_core.h"

namespace vulkan {
	class StaticTexture: public Texture {
		public:
			static util::Result<StaticTexture *, errors::InvalidImageFile> fromFile(
					std::string const &url);
			~StaticTexture();

			VkDescriptorSet getDescriptorSet() const;
			VkImageView imageView() const;
		private:
			StaticTexture() = default;
			VkImage texture_;
			VkDeviceMemory textureMemory_;
			VkImageView textureView_;
			uint32_t mipLevels_;
			VkDescriptorSet imguiDescriptorSet_;
	};
}
