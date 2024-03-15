#pragma once

#include "format.hpp"
#include <optional>
#include <stdexcept>
#include <variant>
namespace util {
	/*
	 * @brief Rust style error handling
	 */
	template<typename Value, typename Error>
		class Result: private std::variant<Value, Error> {
			public:
				Result(Value value): parent_t(std::move(value)) {}
				Result(Error error): parent_t(std::move(error)) {}

				operator bool() const {
					return std::holds_alternative<Value>(*this);
				}

				Value &value() {
					try {
						return std::get<Value>(*this);
					} catch (std::bad_variant_access) {
						throw std::runtime_error(util::f(
									"Trying to get value of function that threw \"",
									error(),
									'"'));
					}
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

				Value &value(Value defaultValue) {
					try {
						return std::get<Value>(*this);
					} catch (std::bad_variant_access) {
						return defaultValue;
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

#define RETURN_IF_ERR(result) if (!result) return {result.error()};
}
