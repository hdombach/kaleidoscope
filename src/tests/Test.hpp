#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <map>
#include <iostream>

#include "util/Env.hpp"
#include "util/log.hpp"
#include "util/KError.hpp"
#include "util/result.hpp"

struct Test;
struct Test {
	std::function<void(Test &)> fn;
	std::string suite_name;
	std::string test_name;
	uint32_t total_test_count = 0;
	uint32_t passed_test_count;
};

using TestSuite = std::map<std::string, Test>;
extern std::map<std::string, TestSuite> suites;

#define _TEST_NAME(pre, suite_name, seperator, test_name) \
test##pre##suite_name##seperator##test_name

#define TEST(suite_name, test_name) \
void _TEST_NAME(_, suite_name, _, test_name)(Test &_test); \
inline struct _TEST_NAME(_class_, suite_name, _, test_name) {\
	_TEST_NAME(_class_, suite_name, _, test_name)() {\
		suites[#suite_name][#test_name] = { \
			_TEST_NAME(_, suite_name, _, test_name), \
			#suite_name, \
			#test_name \
		}; \
	};\
} _TEST_NAME(_inst_, suite_name, _, test_name); \
void _TEST_NAME(_, suite_name, _, test_name)(Test &_test)

inline std::ostream &fail_head(
	Test &test,
	std::source_location loc=std::source_location::current()
) {
	std::cout << "[";
	std::cout << util::color::RED << "FAILED_TEST " << util::color::RESET;
	std::cout	<< std::filesystem::relative(loc.file_name(), util::g_env.working_dir).c_str();
	std::cout << "(" << loc.line() << ":" << loc.column() << ")] ";
	std::cout << test.suite_name << "::" << test.test_name;

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

inline void expect_kerror(
	Test &test,
	KError error,
	KError::Type type,
	const char *lhs_string,
	const char *rhs_string,
	std::source_location loc=std::source_location::current()
) {
	if (error.type() == type) {
		test.passed_test_count++;
	} else {
		auto &os = fail_head(test, loc) << std::endl;

		os << "\tEXPECT_KERROR(" << lhs_string << ", " << rhs_string << ");" << std::endl;
		os << std::endl;
		os << "Mismatching error types. " << error.type() << " != " << type << std::endl;
		os << std::endl;
	}
}

template<typename Val>
inline void expect_kerror(
	Test &test,
	util::Result<Val, KError> result,
	KError::Type type,
	const char *lhs_string,
	const char *rhs_string,
	std::source_location loc=std::source_location::current()
) {
	if (result.has_value()) {
		auto &os = fail_head(test, loc) << std::endl;

		os << "\tEXPECT_KERROR(" << lhs_string << ", " << rhs_string << ");" << std::endl;
		os << std::endl;
		os << "lhs_string Did not return or throw an error" << std::endl;
		os << std::endl;
	} else if (result.error().type() == type) {
		test.passed_test_count++;
	} else {
		auto &os = fail_head(test, loc) << std::endl;

		os << "\tEXPECT_KERROR(" << lhs_string << ", " << rhs_string << ");" << std::endl;
		os << std::endl;
		os << "Mismatching error types. " << result.error().type() << " != " << type << std::endl;
		os << std::endl;
	}
}


#define EXPECT_EQ(lhs, rhs) {\
	_test.total_test_count++;\
	try { \
		expect_eq(_test, lhs, rhs, #lhs, #rhs); \
	} catch (KError const &e) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT_EQ(" #lhs ", " #rhs ");" << std::endl; \
		os << std::endl; \
		os << "\tException was thrown:" << std::endl << "\t"; \
		log_error(e) << std::endl; \
	} catch (std::exception const &e) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT_EQ(" #lhs ", " #rhs ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Exception thrown " << e.what() << std::endl; \
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
		expect(_test, static_cast<bool>(value), #value); \
	} catch (KError const &e) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT(" #value ");" << std::endl; \
		os << std::endl; \
		os << "\tException was thrown:" << std::endl << "\t"; \
		log_error(e) << std::endl; \
	} catch (std::exception const &e) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT(" #value ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Exception thrown " << e.what() << std::endl; \
	} catch (...) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT(" #value ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Unknown exception was thrown" << std::endl; \
	}\
}

#define EXPECT_KERROR(value, kerror_type) {\
	_test.total_test_count++;\
	try { \
		expect_kerror(_test, value, kerror_type, #value, #kerror_type); \
	} catch (KError const &e) { \
		expect_kerror(_test, e, kerror_type, #value, #kerror_type); \
	} catch (std::exception const &e) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\tEXPECT_KERROR(" #value ", " #kerror_type ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Exception thrown " << e.what() << std::endl; \
	} catch (...) { \
		auto &os = fail_head(_test) << std::endl; \
		os << "\ttEXPECT_KERROR(" #value ", " #kerror_type ");" << std::endl; \
		os << std::endl; \
		os << "\t"; \
		log_error() << "Unknown exception was thrown" << std::endl; \
	}\
}


int test_main();
