#include "Error.hpp"

template<>
const char *vulkan::Error::type_str(vulkan::ErrorType t) {
	switch (t) {
		case vulkan::ErrorType::VULKAN:
			return "VulkanError.VULKAN";
		case vulkan::ErrorType::INVALID_ARG:
			return "VulkanError.INVALID_ARG";
		case vulkan::ErrorType::MISC:
			return "VulkanError.MISC";
		case vulkan::ErrorType::INTERNAL:
			return "VulkanError.INTERNAL";
		case vulkan::ErrorType::EMPTY_BUFFER:
			return "VulkanError.EMPTY_BUFFER";
		case vulkan::ErrorType::SHADER_RESOURCE:
			return "VulkanError.SHADER_RESOURCE";
	}
}

std::ostream &operator<<(std::ostream &os, vulkan::ErrorType t) {
	return os << vulkan::Error::type_str(t);
}
