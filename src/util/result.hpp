#pragma once

#include "format.hpp"
#include <optional>
#include <stdexcept>
#include <string>
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

				bool has_value() const {
					return std::holds_alternative<Value>(*this);
				}

				operator bool() const {
					return has_value();
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

				bool has_value() const {
					return !parent_t::has_value();
				}

				operator bool() const {
					return has_value();
				}

				void value() const {
					return;
				}
				Error const &error() const {
					return parent_t::value();
				}

			private:
				using parent_t = std::optional<Error>;

		};

	template<typename Value, typename Error>
		void require(Result<Value, Error> &result) {
			if (!result.has_value()) {
				throw std::runtime_error(std::to_string(result.error()));
			}
		}

#define RETURN_IF_ERR(result) if (!result) return {result.error()};
}
