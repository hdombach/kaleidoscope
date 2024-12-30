#pragma once

#include "format.hpp"
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <type_traits>
#include <vulkan/vulkan_core.h>

namespace util {
	/*
	 * @brief Rust style error handling
	 */
	template<typename Value, typename Error, bool = std::is_reference<Value>::value>
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

				Value &value(Value &default_value) {
					try {
						return std::get<Value>(*this);
					} catch (std::bad_variant_access) {
						return default_value;
					}
				}
				Value const &value(Value const &default_value) const {
					try {
						return std::get<Value>(*this);
					} catch (std::bad_variant_access) {
						return default_value;
					}
				}
				Value value(Value default_value) {
					try {
						return std::get<Value>(*this);
					} catch (std::bad_variant_access) {
						return default_value;
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
		class Result<void, Error, false>: private std::optional<Error> {
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

	template<typename Value>
		class Result<Value, void, false>: private std::optional<Value> {
			public:
				Result() = default;
				Result(Value value): parent_t(value) {}

				bool has_value() const {
					return parent_t::has_value();
				}

				operator bool() const {
					return has_value();
				}

				Value &value() {
					return parent_t::value();
				}
				Value const &value() const {
					return parent_t::value();
				}
				void const error() const {
					return;
				}

			private:
				using parent_t = std::optional<Value>;

		};

	template<typename Value, typename Error>
		class Result<Value, Error, true> {
			public:
				using BaseValue = typename std::remove_reference<Value>::type;
				using PtrValue = BaseValue*;
				using Base = Result<PtrValue, Error>;

			public:
				Result() = default;
				Result(BaseValue value): _base(&value) {}

				template<class ...Args>
					Result(Args ...args): _base(args...) {}

				Result(const Result& other) = delete;
				Result(Result &&other) = default;
				Result& operator=(const Result& other) = delete;
				Result& operator=(Result &&other) = default;

				bool has_value() const {
					return _base.has_value();
				}

				operator bool() const {
					return has_value();
				}

				BaseValue &value() {
					return *_base.value();
				}

				BaseValue const &value() const {
					return *_base.value();
				}

				BaseValue &value(BaseValue default_value) {
					return *_base.value(default_value);
				}

				BaseValue const &value(BaseValue default_value) const {
					return *_base.value(default_value);
				}

				Error const error() const {
					return _base.error();
				}

			private:
				Base _base;
		};

	template<typename Value, typename Error>
		void require(Result<Value, Error> &result) {
			if (!result.has_value()) {
				throw std::runtime_error(std::to_string(result.error()));
			}
		}

	inline void require(VkResult result) {
		if (result != VK_SUCCESS) {
			throw std::runtime_error(std::to_string(result));
		}
	}
}

#define TRY(result) try { return {(result).error()}; } catch (...) {}

#define TRY_LOG(result) try { auto error = result.error(); LOG_ERROR << error << std::endl; } catch (...) {}
