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
			VkImageView image_view() const override;
			uint32_t id() const override;
			void set_name(std::string const &name) override { _name = name; }
			std::string const &name() const override { return _name; }

		private:
			StaticTexture() = default;

		private:
			std::string _name;
			Image _image;
			uint32_t _mip_levels;
			VkDescriptorSet _imgui_descriptor_set;
			uint32_t _id;
	};
}
