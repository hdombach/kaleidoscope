#pragma once

#include <functional>
#include <string>
#include <map>
#include <iostream>

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
	const char *filename,
	size_t line
) {
	return std::cout << "[FAILED TEST " << test.suite_name << "::" << test.test_name
		<< " (" << filename << ":" << line << ")] ";
}

template<typename LHS, typename RHS>
void expect_eq(
	Test &test,
	LHS const &lhs,
	RHS const &rhs,
	const char *lhs_string,
	const char *rhs_string,
	const char *filename,
	size_t line
) {
	if (lhs == rhs) {
		test.passed_test_count++;
	} else {
		auto &os = fail_head(test, filename, line) << std::endl;

		os << "\tEXPECT_EQ(" << lhs_string << ", " << rhs_string << ");" << std::endl;
		os << std::endl;
		os << "\tlhs: " << lhs << std::endl;
		os << "\trhs: " << rhs << std::endl;
		os << std::endl;
	}
}

inline void expect(
	Test &test,
	bool value,
	const char *value_string,
	const char *filename,
	size_t line
) {
	if (value) {
		test.passed_test_count++;
	} else {
		auto &os = fail_head(test, filename, line) << std::endl;

		os << "\tEXPECT(" << value_string << ");" << std::endl;
		os << std::endl;
		os << "\tvalue: " << value << std::endl;
		os << std::endl;
	}
}

#define EXPECT_EQ(lhs, rhs) {\
	_test.total_test_count++;\
	try { \
		expect_eq(_test, lhs, rhs, #lhs, #rhs, __FILE__, __LINE__); \
	} catch (std::exception const &e) { \
		auto &os = fail_head(_test, __FILE__, __LINE__) << std::endl; \
		os << "\tEXPECT_EQ(" #lhs ", " #rhs ");" << std::endl; \
		os << std::endl; \
		os << "\tException was thrown: " << e.what() << std::endl; \
	} catch (...) { \
		auto &os = fail_head(_test, __FILE__, __LINE__) << std::endl; \
		os << "\tEXPECT_EQ(" #lhs ", " #rhs ");" << std::endl; \
		os << std::endl; \
		os << "\tUnknown exception was thrown" << std::endl; \
	}\
}

#define EXPECT(value) {\
	_test.total_test_count++;\
	try { \
		expect(_test, value, #value, __FILE__, __LINE__); \
	} catch (std::exception const &e) { \
		auto &os = fail_head(_test, __FILE__, __LINE__) << std::endl; \
		os << "\tEXPECT(" #value ");" << std::endl; \
		os << std::endl; \
		os << "\tException was thrown: " << e.what() << std::endl; \
	} catch (...) { \
		auto &os = fail_head(_test, __FILE__, __LINE__) << std::endl; \
		os << "\tEXPECT(" #value ");" << std::endl; \
		os << std::endl; \
		os << "\tUnknown exception was thrown" << std::endl; \
	}\
}


int test_main();
