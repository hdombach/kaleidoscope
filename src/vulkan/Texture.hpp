#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <string>

#include "util/log.hpp"


namespace vulkan {
	/*
	 * A texture interface
	 * Has support for both static and dynamic textures (viewports)
	 */
	class Texture {
		public:
			virtual ~Texture() = default;
			virtual VkDescriptorSet imgui_descriptor_set() = 0;
			virtual VkImageView image_view() const = 0;
			virtual uint32_t id() const = 0;
			virtual void set_name(std::string const &name) = 0;
			virtual std::string const &name() const = 0;

			virtual bool is_resizable() const {
				return false;
			}
			virtual void resize(VkExtent2D size) {
				LOG_ERROR << "Cannot resize texture that is not resizable" << std::endl;
			}
	};
}
