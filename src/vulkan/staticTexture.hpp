#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../util/errors.hpp"
#include "../util/result.hpp"
#include "texture.hpp"

namespace vulkan {
	class StaticTexture: public Texture {
		public:
			static util::Result<StaticTexture *, KError> fromFile(
					std::string const &url);
			~StaticTexture();

			VkDescriptorSet getDescriptorSet() const;
			ImageView const &imageView() const;
		private:
			StaticTexture() = default;
			VkImage _texture;
			VkDeviceMemory _textureMemory;
			ImageView _textureView;
			uint32_t _mipLevels;
			VkDescriptorSet _imguiDescriptorSet;
	};
}
