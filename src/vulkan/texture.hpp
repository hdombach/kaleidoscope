#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../util/log.hpp"
#include "imageView.hpp"


namespace vulkan {
	/*
	 * A texture interface
	 * Has support for both static and dynamic textures (viewports)
	 */
	class Texture {
		public:
			virtual ~Texture() = default;
			virtual VkDescriptorSet get_descriptor_set() = 0;
			virtual ImageView const &image_view() = 0;

			virtual bool is_resizable() const {
				return false;
			}
			virtual void resize(VkExtent2D size) {
				util::log_error("Cannot resize texture that is not resizable");
			}
	};
}
