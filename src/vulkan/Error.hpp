#pragma once

#include "util/BaseError.hpp"

namespace vulkan {
	enum class ErrorType {
		VULKAN,
		INVALID_ARG,
		MISC,
		INTERNAL,
		EMPTY_BUFFER,
		SHADER_RESOURCE,
	};

	using Error = TypedError<ErrorType>;
}

template<>
const char *vulkan::Error::type_str(vulkan::ErrorType t);

std::ostream &operator<<(std::ostream &os, vulkan::ErrorType t);
