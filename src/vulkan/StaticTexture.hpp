#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../util/errors.hpp"
#include "../util/result.hpp"
#include "Texture.hpp"
#include "Image.hpp"

namespace vulkan {
	class StaticTexture: public Texture {
		public:
			static util::Result<StaticTexture *, KError> from_file(
					uint32_t id,
					std::string const &url);
			~StaticTexture();

			VkDescriptorSet imgui_descriptor_set() override;
			ImageView const &image_view() override;
			uint32_t id() const override;

		private:
			StaticTexture() = default;

		private:
			Image _texture;
			ImageView _texture_view;
			uint32_t _mip_levels;
			VkDescriptorSet _imgui_descriptor_set;
			uint32_t _id;
	};
}
