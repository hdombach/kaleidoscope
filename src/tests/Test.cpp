#include "Test.hpp"
#include "util/log.hpp"

std::map<std::string, TestSuite> suites;

bool _in_filter(
	std::string const &test_name,
	std::vector<std::string> const &filters
) {
	if (filters.empty()) return true;
	for (auto const &filter : filters) {
		if (test_name.find(filter) != std::string::npos) {
			return true;
		}
	}
	return false;
}

int test_main(std::vector<std::string> const &filters) {
	uint32_t all_total = 0;
	uint32_t all_passed = 0;
	for (auto &suite : suites) {
		log_trace() << "Starting test suite: " << suite.first << std::endl;
		uint32_t suite_total = 0;
		uint32_t suite_passed = 0;
		for (auto &test : suite.second) {
			if (!_in_filter(test.second.full_name(), filters)) {
				log_trace() << "Skipping test: " << test.second.full_name() << std::endl;
				continue;
			}

			try {
				log_trace() << "Starting test: " << test.first << std::endl;
				test.second.fn(test.second);
				auto test_total = test.second.total_test_count;
				auto test_passed = test.second.passed_test_count;

				suite_passed += test_passed;
				suite_total += test_total;
			} catch (KError const &e) {
				suite_total++;
				auto &os = fail_head(test.second) << std::endl;
				os << std::endl;
				os << "\tException was thrown:" << std::endl << "\t";
				log_error(e) << std::endl;
			} catch (std::exception const &e) {
				suite_total++;
				auto &os = fail_head(test.second) << std::endl;
				os << std::endl;
				os << "\tException was thrown:" << std::endl << "\t";
				log_error() << "Exception thrown " << e.what() << std::endl;
			} catch (...) {
				suite_total++;
				log_error() << "Unknown exception was thrown" << std::endl;
			}
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
