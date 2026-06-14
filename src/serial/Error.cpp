#include "Error.hpp"

template<>
const char *serial::Error::type_str(serial::ErrorType t) {
	switch (t) {
		case serial::ErrorType::UNKNOWN:
			return "SerialError.UNKNOWN";
		case serial::ErrorType::PARSE_ERROR:
			return "SerialError.PARSE_ERROR";
		case serial::ErrorType::INVALID_STATE:
			return "SerialError.INVALID_STATE";
		case serial::ErrorType::VALIDATE_ERROR:
			return "SerialError.VALIDATE_ERROR";
	}
}
