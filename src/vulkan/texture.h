#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "log.h"
#include "vulkan/vulkan_core.h"


namespace vulkan {
	/*
	 * A texture interface
	 */
	class Texture {
		public:
			virtual ~Texture() = default;
			virtual VkDescriptorSet getDescriptorSet() const = 0;

			virtual bool isResizable() const {
				return false;
			}
			virtual void resize(glm::ivec2 size) {
				util::log_error("Cannot resize texture that is not resizable");
			}
	};
}
