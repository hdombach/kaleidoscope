
#include "AbsoluteSolver.hpp"
#include "tests/Test.hpp"

namespace cg {
	TEST(AbsoluteSolver, debug_print) {
		auto c = CfgContext();
		c.prim("S") = c["E"];
		c.prim("E") = c["E"] + c.s("*") + c["B"];
		c.prim("E") = c["E"] + c.s("+") + c["B"];
		c.prim("E") = c["B"];
		c.prim("B") = c.s("0");
		c.prim("B") = c.s("1");
		c.simplify();

		auto solver = AbsoluteSolver::setup(c, "S");
		solver->print_table(log_debug() << "\n", {'*', '+', '0', '1'});
	}

	TEST(AbsoluteSolver, simplify_strings) {
		auto c = CfgContext();

		c.prim("opening") = c.s("< ");
		c.prim("closing") = c.s(" >");
		c.prim("message") = c["opening"] + c.s("Hello world") + c["closing"];

		c.prep();
		log_debug() << "before simplifying:\n" << c << std::endl;
		c.simplify();
		log_debug() << "after simplifying:\n" << c << std::endl;

		auto solver = AbsoluteSolver::setup(c, "message");
		solver->print_table(log_debug() << "\n", {' ', '<', '>', 'H', 'e', 'l', 'o', 'r', 'd'});
	}

	TEST(AbsoluteSolver, simplify_sets) {
		auto c = CfgContext();

		c.prim("digit-pair") = c.i("0123456789") + c.s(".") + c.i("1234567890");
		c.prim("str") = c.s("\"") + c.e("\"") + c.s("\"");

		c.prep();
		c.simplify();

		auto solver = AbsoluteSolver::setup(c, "digit-pair");
	}
}
