#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <map>
#include <iostream>

#include "util/PrintTools.hpp"
#include "util/format.hpp"
#include "util/BaseError.hpp"
#include "util/Env.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"
#include "util/result.hpp"

struct Test;

class TestFixture {
	public:
		TestFixture(Test &test): TestFixture(test, 1) {}
		TestFixture(Test &test, size_t variant): _test(test) {};
		static size_t variant_count() { return 1; }

	protected:
		Test &_test;
};

class Test {
	public:
		using Callback = std::function<void(Test &, int variant_index)>;

		Test() = default;

		Test(
			Callback callback,
			std::string const &suite_name,
			std::string const &test_name,
			size_t variant_count = 1
		);

		std::string full_name() const;
		size_t variant_count() const;
		bool test_failed() const;
		bool test_empty() const;
		std::string const &suite_name() const;
		std::string const &test_name() const;


		void operator()(Test &, int variant_index);

		void fail(
			std::vector<std::string> const &msgs,
			util::FileLocation const &loc = std::source_location::current()
		) {
			std::cout << util::color::RED << "TEST FAILED. " << util::color::RESET;
			std::cout << full_name() << " (";
			std::cout	<< std::filesystem::relative(loc.file_name, util::g_env.working_dir).c_str();
			std::cout << ":" << loc.line << ")";
			std::cout << std::endl;

			for (auto &msg : msgs) {
				std::cout << "\t" << msg << std::endl;
			}

			std::cout << std::endl;

			_test_failed = true;
			_test_empty = false;
		}

		void fail(
			std::string const &msg,
			util::FileLocation const &loc = std::source_location::current()
		) {
			return fail(std::vector{msg}, loc);
		}

		void fail(
			std::string const &msg,
			BaseError const &err,
			util::FileLocation const &loc = std::source_location::current()
		) {
			fail(std::vector{msg}, loc);
			std::cout << util::indented(err.str(), "\t") << std::endl;;
		}

		void fail(
			std::vector<std::string> const &msgs,
			BaseError const &err,
			util::FileLocation const &loc = std::source_location::current()
		) {
			fail(msgs, loc);
			std::cout << util::indented(err.str(), "\t") << std::endl;
		}

		template<typename Val, typename Err>
		void expect(
			util::Result<Val, Err> const &val,
			std::vector<std::string> const &msgs = {},
			util::FileLocation const &loc = std::source_location::current()
		) {
			_test_empty = false;
			if (!val.has_value()) {
				auto more_msgs = msgs;
				more_msgs.push_back("Could not unwrap Result");
				fail(more_msgs, val.error(), loc);
			}
		}

		template<typename Val, typename ErrType>
		void expect_terror(
			util::Result<Val, TypedError<ErrType>> const &val,
			ErrType const &t,
			std::vector<std::string> const &msgs = {},
			util::FileLocation const &loc = std::source_location::current()
		) {
			_test_empty = false;
			if (val.has_value()) {
				auto more_msgs = msgs;
				more_msgs.push_back(util::f(
						"Expected error ",
						TypedError<ErrType>::type_str(t),
						" but no error was thrown"
				));
				fail(more_msgs, loc);
			}
		}

		template<typename Lhs, typename Rhs>
		void expect_eq(
			Lhs const &lhs,
			Rhs const &rhs,
			std::vector<std::string> const &msgs = {},
			util::FileLocation const &loc = std::source_location::current()
		) {
			_test_empty = false;
			if (lhs != rhs) {
				auto more_msgs = msgs;
				more_msgs.push_back(util::f("Lhs and rhs are not equal. (", util::abbrev_diff(util::f(lhs), util::f(rhs)), ")"));
				more_msgs.push_back(util::f("lhs: ", std::quoted(util::f(lhs))));
				more_msgs.push_back(util::f("rhs: ", std::quoted(util::f(rhs))));
				fail(more_msgs, loc);
			}
		}

		template<typename Lhs, typename Rhs>
		void expect_eq(
			std::vector<Lhs> const &lhs,
			std::vector<Rhs> const &rhs,
			std::vector<std::string> const &msgs = {},
			util::FileLocation const &loc = std::source_location::current()
		) {
			_test_empty = false;
			int i;
			auto s = std::min(lhs.size(), rhs.size());
			auto m = msgs;
			bool equal = true;
			for (i = 0; i < s; i++) {
				if (lhs[i] != rhs[i]) {
					m.push_back(util::f("Lhs and rhs are not equal at index ", i, " (", lhs[i], " != ", rhs[i], ")"));
					equal = false;
					break;
				}
			}

			if (equal) {
				if (i < lhs.size()) {
					m.push_back(util::f("Lhs is longer than rhs."));
					equal = false;
				} else if (i < rhs.size()) {
					m.push_back(util::f("Rhs is longer than lhs."));
				}
			}

			if (!equal) {
				m.push_back(util::f("lhs: ", util::plist(lhs).str()));
				m.push_back(util::f("rhs: ", util::plist(rhs).str()));
				fail(m, loc);
			}
		}


		template<typename LhsVal, typename LhsErr, typename Rhs>
		void expect_eq(
			util::Result<LhsVal, LhsErr> const &lhs,
			Rhs const &rhs,
			std::vector<std::string> const &msgs = {},
			util::FileLocation const &loc = std::source_location::current()
		) {
			_test_empty = false;
			if (lhs.has_value()) {
				return expect_eq(lhs.value(), rhs);
			} else {
				auto more_msgs = msgs;
				more_msgs.push_back("Could not unwrap lhs");
				fail(more_msgs, lhs.error(), loc);
			}
		}

		template<typename Lhs, typename RhsVal, typename RhsErr>
		void expect_eq(
			Lhs const &lhs,
			util::Result<RhsVal, RhsErr> const &rhs,
			std::vector<std::string> const &msgs = {},
			util::FileLocation const &loc = std::source_location::current()
		) {
			_test_empty = false;
			if (rhs.has_value()) {
				return expect_eq(lhs, rhs.value());
			} else {
				auto more_msgs = msgs;
				more_msgs.push_back("Could not unwrap rhs");
				fail(more_msgs, rhs.error(), loc);
			}
		}

	private:
		Callback _callback;
		std::string _suite_name = "";
		std::string _test_name = "";
		size_t _variant_count = 1;
		bool _test_failed = false;
		bool _test_empty = true;
};

using TestSuite = std::map<std::string, Test>;
extern std::map<std::string, TestSuite> suites;

#define _TEST_NAME(pre, suite_name, seperator, test_name) \
test##pre##suite_name##seperator##test_name

#define TEST(suite_name, test_name) \
void _TEST_NAME(_, suite_name, _, test_name)(Test &_test, int); \
inline struct _TEST_NAME(_class_, suite_name, _, test_name) {\
	_TEST_NAME(_class_, suite_name, _, test_name)() {\
		suites[#suite_name][#test_name] = Test( \
			_TEST_NAME(_, suite_name, _, test_name), \
			#suite_name, \
			#test_name \
		); \
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


/*** main api ***/

#define EXPECT_EQ(lhs, rhs, ...) {\
	try { \
		_test.expect_eq(lhs, rhs __VA_OPT__(,) __VA_ARGS__); \
	} catch (...) { \
		_test.fail("Unexpected exception was thrown"); \
	} \
}

#define EXPECT(value, ...) {\
	try { \
		_test.expect(value __VA_OPT__(,) __VA_ARGS__); \
	} catch (...) { \
		_test.fail("Unexpected exception was thrown"); \
	} \
}

#define EXPECT_TERROR(value, error_type, ...) { \
	try { \
		_test.expect_terror(value, error_type __VA_OPT__(,) __VA_ARGS__); \
	} catch (...) { \
		_test.fail("Unexpected exception was thrown"); \
	} \
}

#define FAIL(msg, err, ...) {\
	try { \
		_test.fail(msg, err __VA_OPT__(,) __VA_ARGS__); \
	} catch (...) { \
		_test.fail("Unexpected exception was thrown"); \
	} \
}

int test_main(std::vector<std::string> const &filters);
