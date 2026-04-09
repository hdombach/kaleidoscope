#include "Test.hpp"
#include "util/log.hpp"

std::map<std::string, TestSuite> suites;

Test::Test(
	Test::Callback callback,
	std::string const &suite_name,
	std::string const &test_name,
	size_t variant_count
):
	_callback(callback),
	_suite_name(suite_name),
	_test_name(test_name),
	_variant_count(variant_count)
{}

std::string Test::full_name() const {
	return _suite_name + "::" + _test_name;
}

size_t Test::variant_count() const {
	return _variant_count;
}

bool Test::test_failed() const {
	return _test_failed;
}

std::string const &Test::suite_name() const {
	return _suite_name;
}

std::string const &Test::test_name() const {
	return _test_name;
}

void Test::operator()(Test &test, int variant_index) {
	_test_failed = false;
	_callback(test, variant_index);
}

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
	uint32_t errors = 0;
	uint32_t total = 0;
	for (auto &suite : suites) {
		log_trace() << "Starting test suite: " << suite.first << std::endl;
		uint32_t suite_total = 0;
		uint32_t suite_passed = 0;
		for (auto &test : suite.second) {
			if (!_in_filter(test.second.full_name(), filters)) {
				log_trace() << "Skipping test: " << test.second.full_name() << std::endl;
				continue;
			}

			for (int i = 0; i < test.second.variant_count(); i++) {
				total++;
				try {
					log_trace() << "Starting test: " << test.first << ":" << i << std::endl;
					test.second(test.second, i);
					
					if (test.second.test_failed()) errors++;
				} catch (...) {
					test.second.fail("Exception was thrown");
					errors++;
				}
			}
		}

	}

	std::cout << "Finished all tests: "
		<< "(" << total - errors << "/" << total << ")" << std::endl;

	if (errors == 0) {
		return 0;
	} else {
		return 1;
	}
}
