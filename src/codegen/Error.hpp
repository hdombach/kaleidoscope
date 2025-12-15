#pragma once

#include "util/BaseError.hpp"

namespace cg {
	enum class ErrorType {
		UNKNOWN,
		MISSING_AST_NODE,
		UNEXPECTED_TOKEN,
		SEMANTIC,
		RUNTIME_CG,
		INVALID_ABS_STACK,
		INTERNAL,
		MISC,
		INVALID_GRAMMAR,
		INVALID_PARSE,
		ASSERT,
	};

	using Error = TypedError<ErrorType>;
}

template<>
const char *cg::Error::type_str(cg::ErrorType t);

std::ostream &operator<<(std::ostream &os, cg::ErrorType t);

#define CG_ASSERT(stm, msg) \
if (!(stm)) {\
	return cg::Error(cg::ErrorType::ASSERT, msg); \
}

