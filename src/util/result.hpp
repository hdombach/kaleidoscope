#pragma once

#include "format.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"
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
		class [[nodiscard]] Result: private std::variant<Value, Error> {
			public:
				Result(Value value): parent_t(std::move(value)) {}
				Result(Error error): parent_t(std::move(error)) {}

				bool has_value() const {
					return std::holds_alternative<Value>(*this);
				}

				explicit operator bool() const {
					return has_value();
				}

				Value &value() {
					log_assert(std::holds_alternative<Value>(*this), "Check has_value() before accessing has_value()");
					return std::get<Value>(*this);
				}

				Value const &value() const {
					log_assert(std::holds_alternative<Value>(*this), "Check has_value() before accessing has_value()");
					return std::get<Value>(*this);
				}

				Value &&move_value() {
					log_assert(std::holds_alternative<Value>(*this), "Check has_value() before accessing has_value()");
					return std::move(std::get<Value>(*this));
				}

				template<typename Target>
				[[nodiscard]] std::optional<Error> move_or(Target &dest) {
					if (has_value()) {
						dest = move_value();
						return {};
					} else {
						return {error()};
					}
				}

				Value *operator->() {
					return &value();
				}
				Value const *operator->() const {
					return &value();
				}


				Value &value(Value &default_value) {
					if (std::holds_alternative<Value>(*this)) {
						return std::get<Value>(*this);
					} else {
						return default_value;
					}
				}
				Value const &value(Value const &default_value) const {
					if (std::holds_alternative<Value>(*this)) {
						return std::get<Value>(*this);
					} else {
						return default_value;
					}
				}
				Value value(Value default_value) {
					if (std::holds_alternative<Value>(*this)) {
						return std::get<Value>(*this);
					} else {
						return default_value;
					}
				}

				Error const &error() const {
					log_assert(std::holds_alternative<Error>(*this), "Check has_value() before accessing error");
					return std::get<Error>(*this);
				}

			private:
				using parent_t = std::variant<Value, Error>;
		};

	template<typename Error>
		class [[nodiscard]] Result<void, Error, false>: private std::optional<Error> {
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
					log_assert(!parent_t::has_value(), "Check has_value() before accessing value()");
				}
				Error const &error() const {
					log_assert(parent_t::has_value(), "Check has_value() before acessing error()");
					return parent_t::value();
				}

				[[nodiscard]] std::optional<Error> move_or() {
					if (has_value()) {
						return {};
					} else {
						return {error()};
					}
				}


			private:
				using parent_t = std::optional<Error>;

		};

	template<typename Value>
		class [[nodiscard]] Result<Value, void, false>: private std::optional<Value> {
			public:
				Result() = default;
				Result(Value value): parent_t(value) {}

				bool has_value() const {
					return parent_t::has_value();
				}

				operator bool() const {
					return has_value();
				}

				Value value() {
					log_assert(has_value(), "Check has_value() before accessing value()");
					return parent_t::value();
				}
				Value const value() const {
					log_assert(has_value(), "Check has_value() before accessing value()");
					return parent_t::value();
				}

				void const error() const {
					log_assert(!has_value(), "Check has_value before accessing error()");
					return;
				}

			private:
				using parent_t = std::optional<Value>;

		};

	template<typename Value, typename Error>
		class [[nodiscard]] Result<Value, Error, true> {
			public:
				using BaseValue = typename std::remove_reference<Value>::type;
				using PtrValue = BaseValue*;
				using Base = Result<PtrValue, Error>;

			public:
				Result() = default;
				Result(Value value): _base(&value) {}
				Result(Error error): _base(error) {}

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

				BaseValue *operator->() {
					return *_base.value();
				}

				BaseValue const *operator->() const {
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
			log_assert(result.has_value(), util::f("Expecting value. got error, ", result.error()));
		}
}


