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
	suites[test.suite_name][test.test_name].total_test_count++;
	if (lhs == rhs) {
		suites[test.suite_name][test.test_name].passed_test_count++;
	} else {
		std::cout << "[FAILED TEST " << test.suite_name << "::" << test.test_name
			<< " (" << filename << ":"<< line << ")] " << std::endl;
		std::cout << "\tEXPECT_EQ(" << lhs_string << ", " << rhs_string << ");" << std::endl;
		std::cout << std::endl;
		std::cout << "\tlhs: " << lhs << std::endl;
		std::cout << "\trhs: " << rhs << std::endl;
		std::cout << std::endl;
	}
}
#define EXPECT_EQ(lhs, rhs) \
expect_eq(_test, lhs, rhs, #lhs, #rhs, __FILE__, __LINE__)

int test_main();
