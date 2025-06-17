
#include "AbsoluteSolver.hpp"
#include "codegen/CfgContext.hpp"
#include "tests/Test.hpp"
#include "util/log.hpp"
#include <fstream>

namespace cg {
	TEST(AbsoluteSolver, debug_print) {
		auto ctx = CfgContext::create();
		auto &c = *ctx;

		c.root("S") = c["E"] + c.eof();
		c.prim("E") = c["E"] + c.s("*") + c["B"];
		c.prim("E") = c["E"] + c.s("+") + c["B"];
		c.prim("E") = c["B"];
		c.prim("B") = c.s("0");
		c.prim("B") = c.s("1");
		c.simplify();

		auto solver = std::move(AbsoluteSolver::create(std::move(ctx)).value());

		std::ofstream file("gen/ast-test.gv");

		solver->print_table(log_debug() << "\n", {'*', '+', '0', '1', '\x03'});
		auto node = solver->parse("1*1", "S");
		node->print_dot(file, "ast-test");
		log_debug() << node.value() << std::endl;
	}

	TEST(AbsoluteSolver, simplify_strings) {
		auto ctx = CfgContext::create();
		auto &c = *ctx;

		c.root("opening") = c.s("< ");
		c.prim("closing") = c.s(" >");
		c.prim("message") = c["opening"] + c.s("Hello world") + c["closing"];

		c.prep().value();
		log_debug() << "before simplifying:\n" << c << std::endl;
		c.simplify();
		log_debug() << "after simplifying:\n" << c << std::endl;

		auto solver = std::move(AbsoluteSolver::create(std::move(ctx)).value());
		solver->print_table(log_debug() << "\n", {' ', '<', '>', 'H', 'e', 'l', 'o', 'r', 'd'});
	}

	TEST(AbsoluteSolver, simplify_sets) {
		auto ctx = CfgContext::create();
		auto &c = *ctx;

		c.root("digit-pair") = c.i("0123456789") + c.s(".") + c.i("1234567890");
		c.prim("str") = c.s("\"") + c.e("\"") + c.s("\"");

		c.prep().value();
		c.simplify();

		auto solver = std::move(AbsoluteSolver::create(std::move(ctx)).value());
	}

	TEST(AbsoluteSolver, lists) {
		auto ctx = CfgContext::create();
		auto &c = *ctx;

		c.root("root") = c["chars"] + c.eof();
		c.prim("chars")
			= c.s("a") + c["chars"]
			| c.s("");

		c.prep().value();
		c.simplify();

		auto solver = std::move(AbsoluteSolver::create(std::move(ctx)).value());

		solver->print_table(log_debug() << "\n", {'a', '\x03'});

		EXPECT_EQ(1, solver->match("a", "root").value());
		EXPECT_EQ(2, solver->match("aa", "root").value());
	}

	TEST(AbsoluteSolver, lines) {
		auto ctx = CfgContext::create();
		auto &c = *ctx;

		//all characters:
		//'{', 'a', '\n', ' '

		c.root("file") = c["lines"] + c.eof();
		c.prim("lines")
			= c["line"] + c["lines"]
			| c.s("");
		c.prim("line")
			= c["line_single"] + c["line"]
			| c.s("\n");

		c.prim("line_single")
			= c["raw"]
			| c["comment"];


		c.temp("raw_opt")
			= c["raw"]
			| c.s("");
		c.prim("raw")
			= c.i("a ") + c["raw_opt"];

		c.prim("comment") =
			c["padding_b"] + c.s("{");

		c.prim("padding")
			= c.s(" ") + c["padding"]
			| c.s("");
		c.prim("padding_b") = c["padding"];

		c.simplify();
		EXPECT(c.prep());

		auto solver = std::move(AbsoluteSolver::create(std::move(ctx)).value());

		solver->print_table(log_debug() << "\n", {'a', '{', '}', ' ', '\n', '\x03'});
		EXPECT_EQ(2, solver->match("a\n", "file").value());

	}
}
