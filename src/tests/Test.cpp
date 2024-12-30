#include "Test.hpp"

std::map<std::string, TestSuite> suites;

int test_main() {
	uint32_t all_total = 0;
	uint32_t all_passed = 0;
	for (auto &suite : suites) {
		uint32_t suite_total = 0;
		uint32_t suite_passed = 0;
		for (auto &test : suite.second) {
			test.second.fn(test.second);
			auto test_total = test.second.total_test_count;
			auto test_passed = test.second.passed_test_count;

			suite_total += test_total;
			suite_passed += test_passed;
		}

		all_total += suite_total;
		all_passed += suite_passed;
	}

	std::cout << "Finished all tests: "
		<< "(" << all_passed << "/" << all_total << ")" << std::endl;

	if (all_passed == all_total) {
		return 0;
	} else {
		return 1;
	}
}
