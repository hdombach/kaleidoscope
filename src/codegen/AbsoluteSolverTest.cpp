
#include "AbsoluteSolver.hpp"
#include "codegen/CfgContext.hpp"
#include "tests/Test.hpp"
#include "util/log.hpp"
#include <fstream>

namespace cg {
	TEST(AbsoluteSolver, debug_print) {
		auto ctx = CfgContext::create();
		auto &c = *ctx;
		using T = Token::Type;

		c.root("root") = c["S"] + T::Eof; 
		c.prim("S") = T::ExpB + c["E"] + T::ExpE;
		c.prim("E") = c["E"] + T::Mult + c["B"];
		c.prim("E") = c["E"] + T::Plus + c["B"];
		c.prim("E") = c["B"];
		c.prim("B") = T::IntConst;
		c.prep().value();
		c.simplify();

		auto solver = std::move(AbsoluteSolver::create(std::move(ctx)).value());
		auto r = solver->parse(util::StringRef("{{1*1+2}}")).value();

		auto ss = std::stringstream();
		r.root_node().print_pre_order(ss);
		EXPECT_EQ(ss.str(), "S ExpB E E E B IntConstant Multiply B IntConstant Plus B IntConstant ExpE ");

		//std::ofstream file("gen/ast-test.gv");
		//node->print_dot(file, "ast-test");
		std::ofstream table("gen/table.txt");
		solver->print_table(table);
	}

	TEST(AbsoluteSolver, simplify_strings) {
		auto ctx = CfgContext::create();
		auto &c = *ctx;
		using T = Token::Type;

		c.root("root") = c["message"] + T::Eof;
		c.prim("opening") = T::Great;
		c.prim("closing") = T::Less;
		c.prim("message") = c["opening"] + T::Ident + c["closing"] + T::Eof;

		c.prep().value();
		log_debug() << "before simplifying:\n" << c << std::endl;
		c.simplify();
		log_debug() << "after simplifying:\n" << c << std::endl;

		auto solver = std::move(AbsoluteSolver::create(std::move(ctx)).value());
		//auto node = solver->parse(util::StringRef"")
		
		//std::ofstream file("gen/ast-test.gv");
		//node->print_dot(file, "ast-test");
	}
}
