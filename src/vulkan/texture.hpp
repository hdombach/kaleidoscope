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
			virtual VkDescriptorSet getDescriptorSet() const = 0;
			virtual ImageView const &imageView() const = 0;

			virtual bool isResizable() const {
				return false;
			}
			virtual void resize(glm::ivec2 size) {
				util::log_error("Cannot resize texture that is not resizable");
			}
	};
}
