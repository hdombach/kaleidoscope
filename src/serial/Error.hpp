#pragma once

#include <iostream>

namespace serial {
	enum class ErrorType {
		UNKNOWN,
		PARSE_ERROR,
		INVALID_STATE,
		VALIDATE_ERROR,
	};
}

std::ostream &operator<<(std::ostream &os, serial::ErrorType t);

#include "util/BaseError.hpp"
namespace serial {
	using Error = TypedError<ErrorType>;
};

template<>
const char *serial::Error::type_str(serial::ErrorType t);
