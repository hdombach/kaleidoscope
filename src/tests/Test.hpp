#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <map>
#include <iostream>

#include "util/BaseError.hpp"
#include "util/Env.hpp"
#include "util/log.hpp"
#include "util/result.hpp"

struct Test;

class TestFixture {
	public:
		TestFixture(Test &test): TestFixture(test, 1) {}
		TestFixture(Test &test, size_t variant) {}
		static size_t variant_count() { return 1; }
};

struct Test {
	std::function<void(Test &, int enum_index)> fn;
	std::string suite_name;
	std::string test_name;
	size_t enum_count = 1;
	uint32_t total_test_count = 0;
	uint32_t passed_test_count = 0;

	std::string full_name() const {
		return suite_name + "::" + test_name;
	}
};

using TestSuite = std::map<std::string, Test>;
extern std::map<std::string, TestSuite> suites;

#define _TEST_NAME(pre, suite_name, seperator, test_name) \
test##pre##suite_name##seperator##test_name

#define TEST(suite_name, test_name) \
void _TEST_NAME(_, suite_name, _, test_name)(Test &_test, int); \
inline struct _TEST_NAME(_class_, suite_name, _, test_name) {\
	_TEST_NAME(_class_, suite_name, _, test_name)() {\
		suites[#suite_name][#test_name] = { \
			_TEST_NAME(_, suite_name, _, test_name), \
			#suite_name, \
			#test_name \
		}; \
	};\
} _TEST_NAME(_inst_, suite_name, _, test_name); \
void _TEST_NAME(_, suite_name, _, test_name)(Test &_test, int)

#define TEST_F(fixture, test_name) \
void _TEST_NAME(_wrapper_, fixture, _, test_name)(Test &_test, int i); \
void _TEST_NAME(_, fixture, _, test_name)(Test &_test, fixture &); \
inline struct _TEST_NAME(_class_, fixture, _, test_name) {\
	_TEST_NAME(_class_, fixture, _, test_name)() {\
		suites[#fixture][#test_name] = { \
			_TEST_NAME(_wrapper_, fixture, _, test_name), \
			#fixture, \
			#test_name, \
			fixture::variant_count() \
		}; \
	};\
} _TEST_NAME(_inst_, fixture, _, test_name); \
void _TEST_NAME(_wrapper_, fixture, _, test_name)(Test &_test, int i) {\
	auto f = fixture(_test, i); \
	_TEST_NAME(_, fixture, _, test_name)(_test, f); \
}\
void _TEST_NAME(_, fixture, _, test_name)(Test &_test, fixture &f)


inline std::ostream &fail_head(
	Test &test,
	std::source_location loc=std::source_location::current()
) {
	std::cout << "[";
	std::cout << util::color::RED << "FAILED_TEST " << util::color::RESET;
	std::cout	<< std::filesystem::relative(loc.file_name(), util::g_env.working_dir).c_str();
	std::cout << "(" << loc.line() << ":" << loc.column() << ")] ";
	std::cout << test.full_name();

	return std::cout;
}

template<typename LHS, typename RHS>
void expect_eq(
	Test &test,
	LHS const &lhs,
	RHS const &rhs,
	const char *lhs_string,
	const char *rhs_string,
	std::source_location loc=std::source_location::current()
) {
	if (lhs == rhs) {
		test.passed_test_count++;
	} else {
		auto &os = fail_head(test, loc) << std::endl;

		os << "\tEXPECT_EQ(" << lhs_string << ", " << rhs_string << ");" << std::endl;
		os << std::endl;
		os << "\tlhs: \"" << lhs << "\"" << std::endl;
		os << "\trhs: \"" << rhs << "\"" << std::endl;
		os << std::endl;
	}
}

inline void expect(
	Test &test,
	bool value,
	const char *value_string,
	std::source_location loc=std::source_location::current()
) {
	if (value) {
		test.passed_test_count++;
	} else {
		auto &os = fail_head(test, loc) << std::endl;

		os << "\tEXPECT(" << value_string << ");" << std::endl;
		os << std::endl;
		os << "\tvalue: " << value << std::endl;
		os << std::endl;
	}
}

template<typename Value, typename Result>
inline void expect(
	Test &test,
	util::Result<Value, Result> const &value,
	const char *value_string,
	std::source_location loc=std::source_location::current()
) {
	if (value.has_value()) {
		test.passed_test_count++;
	} else {
		auto &os = fail_head(test, loc) << std::endl;

		os << "\tEXPECT(" << value_string << ");" << std::endl;
		os << std::endl;
		os << "\terror: " << value.error() << std::endl;
		os << std::endl;
	}
}


template<typename ErrorType>
inline void expect_terror(
	Test &test,
	TypedError<ErrorType> error,
	ErrorType type,
	const char *lhs_string,
	const char *rhs_string,
	std::source_location loc=std::source_location::current()
) {
	if (error.type() == type) {
		test.passed_test_count++;
	} else {
		auto &os = fail_head(test, loc) << std::endl;
		os << "\tEXPECT_TERROR(" << lhs_string << ", " << rhs_string << ");" << std::endl;
		os << std::endl;
		os << "Mismatching error types. " << error.type() << " != " << type << std::endl;
		os << std::endl;
	}
}

template<typename Val, typename ErrorType>
inline void expect_terror(
	Test &test,
	util::Result<Val, TypedError<ErrorType>> result,
	ErrorType type,
	const char *lhs_string,
	const char *rhs_string,
	std::source_location loc=std::source_location::current()
) {
	if (result.has_value()) {
		auto &os = fail_head(test, loc) << std::endl;
		os << "\tEXPECT_TERROR(" << lhs_string << ", " << rhs_string << ");" << std::endl;
		os << std::endl;
		os << lhs_string << " Did not return or throw an error." << std::endl;
		os << std::endl;
	} else if (result.error().type() == type) {
		test.passed_test_count++;
	} else {
		auto &os = fail_head(test, loc) << std::endl;
		os << "EXPECT_TERROR(" << lhs_string << ", " << rhs_string << ");" << std::endl;
		os << std::endl;
		os << "Mismatching error types. " << result.error().type() << " != " << type << std::endl;
		os << std::endl;
	}
}


#define EXPECT_EQ(lhs, rhs) {\
	_test.total_test_count++;\
	try { \
		expect_eq(_test, lhs, rhs, #lhs, #rhs); \
	} catch (std::exception const &e) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT_EQ(" #lhs ", " #rhs ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Exception thrown " << e.what() << std::endl; \
	} catch (BaseError const &e) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT_EQ(" #lhs ", " #rhs ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Exception thrown " << e << std::endl; \
	} catch (...) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT_EQ(" #lhs ", " #rhs ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Unknown exception was thrown" << std::endl; \
	}\
}

#define EXPECT(value) {\
	_test.total_test_count++;\
	try { \
		expect(_test, value, #value); \
	} catch (std::exception const &e) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT(" #value ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Exception thrown " << e.what() << std::endl; \
	} catch (BaseError const &e) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT(" #value ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Exception thrown " << e << std::endl; \
	} catch (...) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT(" #value ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Unknown exception was thrown" << std::endl; \
	}\
}

#define EXPECT_TERROR(value, error_type) {\
	try { \
		_test.total_test_count++;\
		expect_terror(_test, value, error_type, #value, #error_type); \
	} catch (std::exception const &e) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT_KERROR(" #value ", " #error_type ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Exception thrown " << e.what() << std::endl; \
	} catch (BaseError const &e) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT_KERROR(" #value ", " #error_type ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Exception thrown " << e << std::endl; \
	} catch (...) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT_TERROR(" #value ", " #error_type ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Unknown exception was thrown" << std::endl; \
	} \
}

int test_main(std::vector<std::string> const &filters);
