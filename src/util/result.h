#pragma once

#include "format.h"
#include <optional>
#include <stdexcept>
#include <variant>
namespace util {
	/*
	 * I want to try an error system similar
	 * to rust because I feel like I have more
	 * control (vs throwing exceptions)
	 */
	template<typename Value, typename Error>
		class Result: private std::variant<Value, Error> {
			public:
				Result(Value value): parent_t(value) {}
				Result(Error error): parent_t(error) {}

				operator bool() const {
					return std::holds_alternative<Value>(*this);
				}

				Value const &value() const {
					try {
						return std::get<Value>(*this);
					} catch (std::bad_variant_access) {
						throw std::runtime_error(util::f(
									"Trying to get value of function that threw \"",
									error(),
									'"'));
					}
				}

				Value const &value(Value defaultValue) const {
					try {
						return std::get<Value>(*this);
					} catch (std::bad_variant_access) {
						return defaultValue;
					}

				}

				Error const &error() const {
					try {
						return std::get<Error>(*this);
					} catch (std::bad_variant_access) {
						throw std::runtime_error("Cannot get error from function that returned normally");
					}
				}

			private:
				using parent_t = std::variant<Value, Error>;
		};

	template<typename Error>
		class Result<void, Error>: private std::optional<Error> {
			public:
				Result() = default;
				Result(Error error): parent_t(error) {}

				operator bool() const {
					return !*this->has_value;
				}

				void value() const {
					return;
				}
				Error const &error() const {
					return std::get<Error>(*this);
				}

			private:
				using parent_t = std::optional<Error>;

		};
}