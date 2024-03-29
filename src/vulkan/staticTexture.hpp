#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../util/errors.hpp"
#include "../util/result.hpp"
#include "texture.hpp"
#include "image.hpp"

namespace vulkan {
	class StaticTexture: public Texture {
		public:
			static util::Result<StaticTexture *, KError> from_file(
					std::string const &url);
			~StaticTexture();

			VkDescriptorSet get_descriptor_set() override;
			ImageView const &image_view() override;
		private:
			StaticTexture() = default;
			Image _texture;
			ImageView _texture_view;
			uint32_t _mip_levels;
			VkDescriptorSet _imgui_descriptor_set;
	};
}
