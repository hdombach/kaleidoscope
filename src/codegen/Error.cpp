#include "Error.hpp"

template<>
const char *cg::Error::type_str(cg::ErrorType t) {
	switch (t) {
		case cg::ErrorType::UNKNOWN:
			return "ErrorType.UNKNOWN";
		case cg::ErrorType::MISSING_AST_NODE:
			return "ErrorType.MISSING_AST_NODE";
		case cg::ErrorType::UNEXPECTED_TOKEN:
			return "ErrorType.UNEXPECTED_TOKEN";
		case cg::ErrorType::SEMANTIC:
			return "ErrorType.SEMANTIC";
		case cg::ErrorType::RUNTIME_CG:
			return "ErrorType.RUNTIME_CG";
		case cg::ErrorType::INVALID_ABS_STACK:
			return "ErrorType.INVALID_ABS_STACK";
		case cg::ErrorType::INTERNAL:
			return "ErrorType.INTERNAL";
		case cg::ErrorType::MISC:
			return "ErrorType.MISC";
		case cg::ErrorType::INVALID_GRAMMAR:
			return "ErrorType.INVALID_GRAMMAR";
		case cg::ErrorType::INVALID_PARSE:
			return "ErrorType.INVALID_PARSE";
		case cg::ErrorType::ASSERT:
			return "ErrorType.ASSERT";
	}
}

std::ostream &operator<<(std::ostream &os, cg::ErrorType t) {
	return os << cg::Error::type_str(t);
}
