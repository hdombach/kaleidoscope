#include "TemplGen.hpp"
#include "codegen/AstNode.hpp"
#include "codegen/CfgContext.hpp"
#include "codegen/SParser.hpp"
#include "codegen/TemplObj.hpp"
#include "util/KError.hpp"
#include "util/IterAdapter.hpp"
#include "util/file.hpp"
#include "util/log.hpp"
#include "util/PrintTools.hpp"
#include "util/lines_iterator.hpp"
#include "ParserContext.hpp"
#include "AstNodeIterator.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>

/**
 * Timing
 *
 * SParser
 * Initial: 7918ms
 *
 * AbsoluteSolver
 *
 * Initial 35920ms
 * Remove debugging 11775ms
 * After using linked list in AstNode: 1704 ms (278ms with optimization).
 *
 * Final speed up. 2125ms (branch 23ff4d8 ) to 200ms (branch 9a8d6f1)
 */

namespace cg {
	Parser::Ptr TemplGen::_parser;

	util::Result<void, KError> TemplGen::_setup_parser() {
		if (_parser) return {};

		auto context = CfgContext::create();
		auto &c = *context;
		using T = Token::Type;

		c.prim("whitespace")
			= T::Pad + c["whitespace"]
			| T::Newline + c["whitespace"]
			| c.empty();

		c.prim("line_single")
			= c["statement"]
			| c["expression"]
			| c["comment"]
			| T::Pad
			| T::Unmatched
			| T::Newline;
		c.prim("line")
			= c["line_single"] + c["line"]
			| c.empty();

		c.prim("lines")
			= c["line_single"] + c["lines"]
			| c.empty();

		c.root("file") = c["lines"] + T::Eof;

		c.temp("comment_cls")
			= T::Pad + c["comment_cls"]
			| T::Unmatched + c["comment_cls"]
			| T::Newline + c["comment_cls"]
			| c.empty();

		c.prim("comment") =
			T::CommentB + c["comment_cls"] + T::CommentE;

		c.prim("expression") =
			T::ExpB +
			c["whitespace"] +
			c["exp"] +
			T::ExpE;

		c.prim("exp_sing")
			= T::Ident + c["whitespace"]
			| T::IntConst + c["whitespace"]
			| T::StrConst + c["whitespace"]
			| c["exp_paran"] + c["whitespace"];

		c.temp("exp_paran") = T::ParanOpen + c["whitespace"] + c["exp"] + T::ParanClose;

		c.temp("exp1_cls")
			= c["exp_member"] + c["exp1_cls"]
			| c["exp_call"] + c["exp1_cls"]
			| c.empty();
		c.prim("exp1") = c["exp_sing"] + c["exp1_cls"];
		c.prim("exp_member") = T::Period + c["whitespace"] + T::Ident;
		c.temp("exp_call_args_cls")
			= T::Comma + c["whitespace"] + c["exp"] + c["exp_call_args_cls"]
			| c.empty();
		c.temp("exp_call_args")
			= c["exp"] + c["exp_call_args_cls"]
			| c.empty();
		c.prim("exp_call") = T::ParanOpen + c["whitespace"] + c["exp_call_args"] + T::ParanClose + c["whitespace"];

		c.prim("exp2")
			= c["exp_plus"] + c["whitespace"]
			| c["exp_min"] + c["whitespace"]
			| c["exp_log_not"] + c["whitespace"]
			| c["exp1"] + c["whitespace"];

		c.prim("exp_plus") = T::Plus + c["whitespace"] + c["exp2"];
		c.prim("exp_min") = T::Minus + c["whitespace"] + c["exp2"];
		c.prim("exp_log_not") = T::Excl + c["whitespace"] + c["exp2"];

		c.temp("exp3_cls")
			= c["exp_mult"] + c["exp3_cls"]
			| c["exp_div"] + c["exp3_cls"]
			| c["exp_mod"] + c["exp3_cls"]
			| c.empty();
		c.prim("exp3") = c["exp2"] + c["exp3_cls"] + c["whitespace"];
		c.prim("exp_mult") = T::Mult + c["whitespace"] + c["exp2"];
		c.prim("exp_div")  = T::Div + c["whitespace"] + c["exp2"];
		c.prim("exp_mod")  = T::Perc + c["whitespace"] + c["exp2"];


		c.temp("exp4_cls")
			= c["exp_add"] + c["exp4_cls"]
			| c["exp_sub"] + c["exp4_cls"]
			| c.empty();
		c.prim("exp4") = c["exp3"] + c["exp4_cls"] + c["whitespace"];
		c.prim("exp_add") = T::Plus + c["whitespace"] + c["exp3"];
		c.prim("exp_sub") = T::Minus + c["whitespace"] + c["exp3"];

		c.temp("exp6_cls")
			= c["exp_comp_g"] + c["exp6_cls"]
			| c["exp_comp_ge"] + c["exp6_cls"]
			| c["exp_comp_l"] + c["exp6_cls"]
			| c["exp_comp_le"] + c["exp6_cls"]
			| c.empty();
		c.prim("exp6") = c["exp4"] + c["exp6_cls"] + c["whitespace"];
		c.prim("exp_comp_g") = T::Great + c["whitespace"] + c["exp4"];
		c.prim("exp_comp_ge") = T::GreatEq + c["whitespace"] + c["exp4"];
		c.prim("exp_comp_l") = T::Less + c["whitespace"] + c["exp4"];
		c.prim("exp_comp_le") = T::LessEq + c["whitespace"] + c["exp4"];

		c.temp("exp7_cls")
			= c["exp_comp_eq"] + c["exp7_cls"]
			| c["exp_comp_neq"] + c["exp7_cls"]
			| c.empty();
		c.prim("exp7") = c["exp6"] + c["exp7_cls"] + c["whitespace"];
		c.prim("exp_comp_eq") = T::Equal + c["whitespace"] + c["exp6"];
		c.prim("exp_comp_neq") = T::NotEqual + c["whitespace"] + c["exp6"];

		c.temp("exp11_cls")
			= c["exp_log_and"] + c["exp11_cls"]
			| c.empty();
		c.prim("exp11") = c["exp7"] + c["exp11_cls"] + c["whitespace"];
		c.prim("exp_log_and") = T::LAnd + c["whitespace"] + c["exp7"];

		c.temp("exp12_cls")
			= c["exp_log_or"] + c["exp12_cls"]
			| c.empty();
		c.prim("exp12") = c["exp11"] + c["exp12_cls"] + c["whitespace"];
		c.prim("exp_log_or") = T::LOr + c["whitespace"] + c["exp11"];

		c.prim("exp_filter_frag")
			= T::Ident + c["exp_call"]
			| T::Ident;

		c.temp("exp_filter_cls")
			= T::Bar + c["exp_filter_frag"] + c["exp_filter_cls"]
			| c.empty();
		c.prim("exp_filter") =
			c["exp12"] + c["exp_filter_cls"] +
			c["whitespace"];

		c.prim("exp") = c["exp_filter"];

		c.prim("sfrag_else") =
			T::StmtB +
			c["whitespace"] + T::Else + c["whitespace"] +
			T::StmtE;

		c.prim("sfrag_if") =
			T::StmtB +
			c["whitespace"] + T::If + c["whitespace"] + c["exp"] + c["whitespace"] +
			T::StmtE;
		c.prim("sfrag_elif") =
			T::StmtB +
			c["whitespace"] + T::Elif + c["whitespace"] + c["exp"] + c["whitespace"] +
			T::StmtE;
		c.prim("sfrag_endif") =
			T::StmtB +
			c["whitespace"] + T::Endif + c["whitespace"] +
			T::StmtE;
		c.prim("sif_start_chain") = c["sfrag_if"] + c["sif_start_chain_cls"];
		c.temp("sif_start_chain_cls")
			= c["line_single"] + c["sif_start_chain_cls"]
			| c["sfrag_elif"] + c["sif_start_chain_cls"]
			| c["sfrag_else"] + c["sif_else_chain_cls"]
			| c["sfrag_endif"];

		c.temp("sif_else_chain_cls")
			= c["line_single"] + c["sif_else_chain_cls"]
			| c["sfrag_endif"];
		c.prim("sif") =
			c["sif_start_chain"];

		c.prim("sfrag_for") =
			T::StmtB +
			c["whitespace"] + T::For +
			c["whitespace"] + T::Ident + c["whitespace"] +
			T::In + c["whitespace"] + c["exp"] +
			T::StmtE;
		c.prim("sfrag_endfor") =
			T::StmtB +
			c["whitespace"] + T::EndFor +  c["whitespace"] +
			T::StmtE;
		c.prim("sfor") = c["sfrag_for"] + c["sfor_lines"];
		c.prim("sfor_lines")
			= c["line_single"] + c["sfor_lines"]
			| c["sfrag_endfor"];


		c.temp("sfrag_argdef_cls")
			= T::Comma + c["whitespace"] + c["sfrag_argdef"] + c["sfrag_argdef_cls"]
			| c.empty();
		c.prim("sfrag_argdef_list")
			= c["sfrag_argdef"] + c["sfrag_argdef_cls"]
			| c.empty();
		c.prim("sfrag_argdef")
			= T::Ident + c["whitespace"] + T::Assignment + c["whitespace"] + c["exp"]
			| T::Ident + c["whitespace"];

		c.prim("sfrag_macro") =
			T::StmtB +
			c["whitespace"] + T::Macro +
			c["whitespace"] + T::Ident + c["whitespace"] +
			T::ParanOpen + c["whitespace"] + c["sfrag_argdef_list"] + T::ParanClose + c["whitespace"] +
			T::StmtE;
		c.prim("sfrag_endmacro") =
			T::StmtB +
			c["whitespace"] + T::Endmacro + c["whitespace"] +
			T::StmtE;
		c.prim("smacro") = c["sfrag_macro"] + c["smacro_lines"];

		c.prim("smacro_lines")
			= c["line_single"] + c["smacro_lines"]
			| c["sfrag_endmacro"];

		c.prim("sinclude") =
			T::StmtB +
			c["whitespace"] + T::Include +
			c["whitespace"] + T::StrConst + c["whitespace"] +
			T::StmtE;

		c.prim("statement") = c["sfor"] | c["sif"] | c["smacro"] | c["sinclude"];

		if (true) {
			TRY(c.prep());
			c.simplify();
			//std::ofstream file("gen/nothing-table.txt");
			auto parser = std::move(AbsoluteSolver::create(std::move(context)).value());
			//parser->print_table(file);
			_parser = std::move(parser);
		} else {
			TRY(c.prep());
			c.simplify();

			auto parser = SParser::create(std::move(context));
			_parser = std::move(parser);
		}

		return {};
	}

	TemplGen::TemplGen(TemplGen &&other) {
		_parser = std::move(other._parser);
	}

	TemplGen &TemplGen::operator=(TemplGen &&other) {
		_parser = std::move(other._parser);
		return *this;
	}

	util::Result<std::string, KError> TemplGen::codegen(
		std::string const &str,
		TemplObj const &args,
		std::string const &filename
	) {
		if (auto dict = args.dict()) {
			return codegen(str, dict.value(), filename);
		} else {
			return dict.error();
		}
	}

	util::Result<std::string, KError> TemplGen::codegen(
		std::string const &str,
		TemplDict const &args,
		std::string const &filename
	) {
		try {
			_setup_parser();
			auto t = TemplGen();
			std::ofstream file("gen/templgen.gv");
			auto label = util::f("Graph for file: ", filename);

			//auto parser = AbsoluteSolver::setup(_ctx, "file").value();

			auto node = _parser->parse({str.c_str(), filename.c_str()}, t._parser_result);
			node.value()->compress(_parser->cfg().prim_names());

			node.value()->print_dot(file, label);

			file.close();

			auto l_args = args;
			TRY(t._add_builtin_identifiers(l_args));
			return t._codegen(*node.value(), l_args);
		} catch_kerror;
	}

	#define CG_ASSERT(stm, msg) \
	if (!(stm)) {\
		return KError::codegen(msg); \
	}

	util::Result<std::string, KError> _unpack_str(std::string const &str) {
		auto s = std::string();
		auto c = str.c_str();

		CG_ASSERT(*c == '"', "String literal must start with '\"'");
		c++;
		while (*c != '"') {
			if (*c == '\0') {
				return KError::codegen("Unexpected end to string sequence");
			}

			if (*c == '\\') {
				c++;
				switch (*c) {
					case '"':
						s += '"';
						break;
					default:
						return KError::codegen(util::f(
							"Unknown string escape sequence: \\",
							*c
						));
				}
			} else {
				s += *c;
			}
			c++;
		}
		return s;
	}

	util::Result<std::string, KError> TemplGen::_codegen(
		AstNode const &node,
		TemplDict &args
	) {
		if (node.type() == AstNode::Type::Leaf) {
			return node.tok().content();
		} else if (node.type() == AstNode::Type::None) {
			return {""};
		}

		if (node.cfg_rule() == "whitespace") {
			return _cg_default(node, args);
		} else if (node.tok().type() == Token::Type::Pad) {
			return _cg_recursive(node, args);
		} else if (node.tok().type() == Token::Type::Ident) {
			return _cg_identifier(node, args);
		} else if (node.cfg_rule() == "raw") {
			return _cg_recursive(node, args);
		} else if (node.cfg_rule() == "line_single") {
			return _cg_line(node, args);
		} else if (node.cfg_rule() == "lines") {
			return _cg_lines(node, args);
		} else if (node.cfg_rule() == "smacro_lines") {
			return _cg_lines(node, args);
		} else if (node.cfg_rule() == "sfor_lines") {
			return _cg_lines(node, args);
		} else if (node.cfg_rule() == "file") {
			return _cg_ref(node, args, 2);
		} else if (node.cfg_rule() == "comment") {
			return _cg_comment(node, args);
		} else if (node.cfg_rule() == "expression") {
			return _cg_expression(node, args);
		} else if (node.cfg_rule() == "statement") {
			return _cg_ref(node, args);
		} else if (node.cfg_rule() == "sif") {
			return _cg_sif(node, args);
		} else if (node.cfg_rule() == "sfor") {
			return _cg_sfor(node, args);
		} else if (node.cfg_rule() == "smacro") {
			TRY(_cg_smacro(node, args));
			return {""};
		} else if (node.cfg_rule() == "sinclude") {
			return _cg_sinclude(node, args);
		} else if (node.cfg_rule() == "sfrag_endmacro") {
			return {""};
		} else if (node.cfg_rule() == "sfrag_endfor") {
			return {""};
		} else {
			return KError::codegen("Unimplimented AstNode type: " + std::string(node.cfg_rule()));
		}
	}

	util::Result<std::string, KError> TemplGen::_cg_default(
		AstNode const &node,
		TemplDict &args
	) {
		return node.consumed_all();
	}

	TemplGen::CodegenRes TemplGen::_cg_recursive(
		AstNode const &node,
		TemplDict &args
	) {
		try {
			auto r = node.tok().content();
			for (auto &child : node) {
				r += _codegen(child, args).value();
			}
			return r;
		} catch_kerror;
	}

	util::Result<std::string, KError> TemplGen::_cg_ref(
		AstNode const &node,
		TemplDict &args,
		size_t count
	) {
		//CG_ASSERT(node.children().size() == count, "Children count of codegen_ref must be 1");
		return _codegen(*node.begin(), args);
	}

	util::Result<std::string, KError> TemplGen::_cg_identifier(
		AstNode const &node,
		TemplDict &args
	) {
		return KError::codegen("Identifier does not have a codegen implimentation");
	}

	util::Result<std::string, KError> TemplGen::_cg_line(
		AstNode const &node,
		TemplDict &args
	) {
		auto result = std::string();
		for (auto &child : node) {
			if (auto str = _codegen(child, args)) {
				result += str.value();
			} else {
				return str;
			}
		}
		return result;
	}

	util::Result<std::string, KError> TemplGen::_cg_lines(
		AstNode const &node,
		TemplDict &args
	) {
		auto result = std::string();
		for (auto &child : node) {
			if (auto str = _codegen(child, args)) {
				result += str.value();
			} else {
				return str;
			}
		}
		return result;
	}

	util::Result<std::string, KError> TemplGen::_cg_comment(
		AstNode const &node,
		TemplDict &args
	) {
		return {""};
	}

	util::Result<std::string, KError> TemplGen::_cg_expression(
		AstNode const &node,
		TemplDict &args
	) {
		try {
			auto result = std::string();

			for (auto &child : node) {
				auto name = child.cfg_rule();
				if (name == "whitespace") {
					continue;
				} else if (child.tok().type() == Token::Type::ExpB) {
					//TODO: padding
					continue;
				} else if (child.tok().type() == Token::Type::ExpE) {
					//TODO: padding
					continue;
				} else {
					auto obj = _eval(child, args);
					result += obj->str().value();
				}
			}
			return result;
		} catch_kerror;
	}

	util::Result<std::string, KError> TemplGen::_cg_statement(
		AstNode const &node,
		TemplDict &args
	) {
		return KError::codegen("statment not implimented");
	}

	util::Result<std::string, KError> TemplGen::_cg_sif(
		AstNode const &node,
		TemplDict &args
	) {
		try {
		auto result = std::string();
		CG_ASSERT(node.cfg_rule() == "sif", "INTERNAL func can only parser sif nodes");

		auto sif_chain = node.child_with_cfg("sif_start_chain").value();

		int i = 0;
		bool cg_current_block = false;
		TemplDict block_args;
		//TODO
		while (i < sif_chain->child_count()) {
			auto &child = sif_chain->begin()[i];
			if (cg_current_block) {
				if (child.cfg_rule() == "line_single") {
					result += _codegen(child, block_args).value();
				} else {
					break;
				}
			} else if (child.cfg_rule() == "sfrag_if") {
				auto exp_node = child.child_with_cfg("exp").value();

				auto bool_value = _eval(*exp_node, args).value();
				if (bool_value.boolean().value()) {
					block_args = args;
					cg_current_block = true;
				}
			} else if (child.cfg_rule() == "sfrag_elif") {
				auto exp_node = child.child_with_cfg("exp").value();

				auto bool_value = _eval(*exp_node, args).value();
				if (bool_value.boolean().value()) {
					block_args = args;
					cg_current_block = true;
				}
			} else if (child.cfg_rule() == "sfrag_else") {
				block_args = args;
				cg_current_block = true;
			} else if (child.cfg_rule() == "sfrag_endif") {
				break;
			}
			i++;
		}

		return result;
		} catch_kerror;
	}

	util::Result<std::string, KError> TemplGen::_cg_sfor(
		AstNode const &node,
		TemplDict &args
	) {
		try {
			auto result = std::string();


			auto sfrag_for = node.child_with_cfg("sfrag_for").value();
			auto iter_name = sfrag_for
				->child_with_tok(Token::Type::Ident).value()
				->tok().content();
			auto iter = sfrag_for->child_with_cfg("exp").value();
			auto lines = node.child_with_cfg("sfor_lines").value();

			auto iter_obj = _eval(*iter, args)->list().value();
			int index = 0;
			for (auto &i : iter_obj) {
				auto loop = TemplObj{
					{"index", index+1},
					{"index0", index},
					{"first", index==0},
					{"last", index==iter_obj.size()-1}
				};

				auto local_args = args;
				local_args[iter_name] = i;
				local_args["loop"] = loop;
				result += _codegen(*lines, local_args).value();
				index++;
			}

			return result;
		} catch_kerror;
	}

	util::Result<void, KError> TemplGen::_cg_smacro(
		AstNode const &node,
		TemplDict &args
	) {
		try {
			auto arg_def = node.child_with_cfg("sfrag_macro").value();
			auto macro_name = arg_def->child_with_tok(Token::Type::Ident).value()->tok().content();
			auto macro_arg_list = arg_def->child_with_cfg("sfrag_argdef_list").value(nullptr);
			auto content = node.child_with_cfg("smacro_lines").value();

			if (args.contains(macro_name)) {
				return KError::codegen(util::f(
					"Cannot create macro with name ",
					macro_name,
					" because identifier already exists"
				));
			}

			auto macro_args = std::vector<std::tuple<std::string, TemplObj>>();
			if (macro_arg_list) {
				for (auto &macro_arg_node : macro_arg_list->children_with_cfg("sfrag_argdef")) {
					auto macro_arg_name = macro_arg_node->child_with_tok(Token::Type::Ident).value()->tok().content();
					auto macro_arg_value = TemplObj();
					if (auto exp_node = macro_arg_node->child_with_cfg("exp")) {
						macro_arg_value = _eval(*exp_node.value(), args).value();
					}
					macro_args.push_back({macro_arg_name, macro_arg_value});
				}
			}
			
			TemplFunc func = [this, macro_args, args, macro_name, content](TemplList l) -> TemplFuncRes {
				auto local_args = args;
				if (macro_args.size() < l.size()) {
					return KError::codegen(util::f(
						"Too many arguments provided to macro ",
						macro_name,
						". ",
						macro_args.size(),
						" expected, ",
						l.size(),
						" received."
					));
				}
				int i = 0;
				for (auto &arg_node : macro_args) {
					auto &[arg_name, arg_value] = arg_node;
					if (i < l.size()) {
						local_args[arg_name] = l[i];
					} else {
						if (arg_value.type() == TemplObj::Type::None) {
							return KError::codegen(util::f(
								"Not enough arguments passed to macro:",
								macro_name
							));
						} else {
							local_args[arg_name] = arg_value;
						}
					}
					i++;
				}
				return {_codegen(*content, local_args).value()};
			};
			args[macro_name] = func;
			return {};
		} catch_kerror;
	}

	TemplGen::CodegenRes TemplGen::_cg_sinclude(
		AstNode const &node,
		TemplDict &args
	) {
		try {
			auto filename = _unpack_str(node.child_with_tok(Token::Type::StrConst).value()->tok().content()).value();
			auto include_src = util::readEnvFile(filename);
			auto include_node = _parser->parse({include_src.c_str(), filename.c_str()}, _parser_result).value();
			include_node->compress(_parser->cfg().prim_names());


			std::ofstream file("gen/templgen-include.gv");
			include_node->print_dot(file, "templgen-include");
			file.close();

			return _codegen(*include_node, args);
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval(
		util::Result<AstNode, KError> const &node,
		TemplDict const &args
	) {
		if (node.has_value()) {
			return _eval(node.value(), args);
		} else {
			return node.error();
		}
	}

	TemplGen::EvalRes TemplGen::_eval(
		AstNode const &node,
		TemplDict const &args
	) {
		auto name = node.cfg_rule();
		if (name == "exp") {
			return _eval(node.begin()[0], args);
		} else if (name == "exp_sing") {
			return _eval_exp_sing(node, args);
		} else if (name == "exp1") {
			return _eval_exp1(node, args);
		} else if (name == "exp2") {
			return _eval_exp2(node, args);
		} else if (name == "exp3") {
			return _eval_exp3(node, args);
		} else if (name == "exp4") {
			return _eval_exp4(node, args);
		} else if (name == "exp6") {
			return _eval_exp6(node, args);
		} else if (name == "exp7") {
			return _eval_exp7(node, args);
		} else if (name == "exp11") {
			return _eval_exp11(node, args);
		} else if (name == "exp12") {
			return _eval_exp12(node, args);
		} else if (name == "exp_filter") {
			return _eval_filter(node, args);
		} else {
			return KError::codegen("Unimplimented AstNode type: " + std::string(node.cfg_rule()));
		}
	}

	util::Result<TemplObj, KError> TemplGen::_eval_exp_sing(
		AstNode const &node,
		TemplDict const &args
	) {
		CG_ASSERT(node.cfg_rule() == "exp_sing", "_eval_exp_sing must be used to parse exp_sing nodes");
		CG_ASSERT(node.type() == AstNode::Type::Rule, "_eval_exp_sing must be of type Rule");
		for (auto &child : node) {
			//TODO
			//if (child.type() == AstNode::Type::Token) continue;
			auto name = child.cfg_rule();
			if (name == "whitespace") {
				continue;
			} else if (child.tok().type() == Token::Type::ParanOpen) {
				continue;
			} else if (child.tok().type() == Token::Type::ParanClose) {
				continue;
			} else if (child.tok().type() == Token::Type::Ident) {
				return _eval_exp_id(child, args);
			} else if (child.tok().type() == Token::Type::IntConst) {
				return _eval_exp_int(child, args);
			} else if (child.tok().type() == Token::Type::StrConst) {
				return _eval_exp_str(child, args);
			} else if (name == "exp") {
				return _eval(child, args);
			} else {
				return KError::codegen("Unknown node passed to _eval_exp_sing: " + child.str());
			}
		}
		return KError::codegen("exp_sing is an empty node");
	}
	util::Result<TemplObj, KError> TemplGen::_eval_exp_id(
		AstNode const &node,
		TemplDict const &args
	) {
		try {
			auto name = node.tok().content();
			if (args.count(name) == 0) {
				log_debug() << "unknown identifier " << node << std::endl;
				return KError::codegen("Unknown identifier \"" + name + "\"", node.location());
			}
			return args.at(name).dup().set_location(node.location());
		} catch_kerror;
	}
	util::Result<TemplObj, KError> TemplGen::_eval_exp_int(
		AstNode const &node,
		TemplDict const &args
	) {
		try {
			CG_ASSERT(node.tok().type() == Token::Type::IntConst, "IntConst must be passed to _eval_exp_int");
			auto value = TemplInt(0);
			for (auto c : node.consumed_all()) {
				value = value * 10 + c - '0';
			}
			auto value_obj = TemplObj(value).set_location(node.location());
			return {value_obj};
		} catch_kerror;
	}

	util::Result<TemplObj, KError> TemplGen::_eval_exp_str(
		AstNode const &node,
		TemplDict const &args
	) {
		try {
			return TemplObj(_unpack_str(node.consumed_all()).value())
				.set_location(node.location());
		} catch_kerror;
	}

	util::Result<TemplObj, KError> TemplGen::_eval_exp1(
		AstNode const &node,
		TemplDict const &args
	) {
		TemplDict l_args = args;
		try {
			auto exp = node.child_with_cfg("exp_sing").value();
			auto res = _eval(*exp, l_args);
			CG_ASSERT(node.begin() != node.end(), "Node must have at least one child");

			auto start = node.begin();
			start++;
			auto children = util::Adapt(start, node.end());
			for (auto const &child : children) {
				//TODO
				//if (child.type() == AstNode::Type::Token) continue;
				if (child.cfg_rule() == "exp_member") {
					l_args["self"] = res.value();
					res = _eval_exp_member(res.value(), child, l_args);
				} else if (child.cfg_rule() == "exp_call") {
					res = _eval_exp_call(res.value(), child, l_args);
				} else {
					return KError::codegen("Unrecognized cfg node: " + std::string(child.cfg_rule()));
				}
			}
			return res;
		} catch_kerror;
	}

	util::Result<TemplObj, KError> TemplGen::_eval_exp_member(
		TemplObj const &lhs,
		AstNode const &node,
		TemplDict const &args
	) {
		try {
			auto name = node.child_with_tok(Token::Type::Ident).value()->tok().content();
			return lhs.get_attribute(name);
		} catch_kerror;
	}

	util::Result<TemplObj, KError> TemplGen::_eval_exp_call(
		TemplObj const &lhs,
		AstNode const &node,
		TemplDict const &args
	) {
		try {
			auto call_args = TemplList();
			if (args.contains("self")) {
				call_args.push_back(args.at("self"));
			}
			for (auto const &child : node) {
				if (child.type() == AstNode::Type::Leaf) continue;
				if (child.cfg_rule() == "whitespace") continue;
				CG_ASSERT(child.cfg_rule() == "exp", "Function call list must have exp nodes");
				call_args.push_back(_eval(child, args).value());
			}
			return lhs.func().value()(call_args);
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_exp2(
		AstNode const &node,
		TemplDict const &args
	) {
		for (auto &child : node) {
			auto name = child.cfg_rule();
			auto exp2 = child.child_with_cfg("exp2");

			if (name == "whitespace") {
				continue;
			} else if (name == "exp_plus") {
				return +_eval(*exp2.value(), args);
			} else if (name == "exp_min") {
				return -_eval(*exp2.value(), args);
			} else if (name == "exp_log_not") {
				return !_eval(*exp2.value(), args);
			} else if (name == "exp1") {
				return _eval(child, args);
			} else {
				return KError::codegen("Unknown child in _eval_exp2: " + std::string(name));
			}
		}
		return KError::codegen("Empty node passed to _eval_exp2");
	}

	TemplGen::EvalRes TemplGen::_eval_exp3(
		AstNode const &node,
		TemplDict const &args
	) {
		try {
			auto exp = node.child_with_cfg("exp2").value();
			auto res = _eval(*exp, args);

			for (auto &child : node) {
				//TODO
				//if (child.type() == AstNode::Type::Token) continue;
				auto name = child.cfg_rule();
				auto exp2 = child.child_with_cfg("exp2");

				if (name == "whitespace") {
					continue;
				} else if (name == "exp2") {
					continue;
				} else if (name == "exp_mult") {
					res = res * _eval(*exp2.value(), args);
				} else if (name == "exp_div") {
					res = res / _eval(*exp2.value(), args);
				} else if (name == "exp_mod") {
					res = res % _eval(*exp2.value(), args);
				} else {
					return KError::codegen("Unknown child in _eval_exp3: " + std::string(name));
				}
			}
			return res;
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_exp4(
		AstNode const &node,
		TemplDict const &args
	) {
		try {
			auto exp = node.child_with_cfg("exp3").value();
			auto res = _eval(*exp, args);

			for (auto &child : node) {
				//TODO
				//if (child.type() == AstNode::Type::Token) continue;
				auto name = child.cfg_rule();
				auto exp3 = child.child_with_cfg("exp3");

				if (name == "whitespace") {
					continue;
				} else if (name == "exp3") {
					continue;
				} else if (name == "exp_add") {
					res = res + _eval(*exp3.value(), args);
				} else if (name == "exp_sub") {
					res = res - _eval(*exp3.value(), args);
				} else {
					return KError::codegen("Unknown child in _eval_exp4: " + std::string(name));
				}
			}
			return res;
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_exp6(
		AstNode const &node,
		TemplDict const &args
	) {
		try {
			auto exp = node.child_with_cfg("exp4").value();
			auto res = _eval(*exp, args);

			for (auto &child : node) {
				//TODO
				//if (child.type() == AstNode::Type::Token) continue;
				auto name = child.cfg_rule();
				auto exp4 = child.child_with_cfg("exp4");
				if (name == "whitespace") {
					continue;
				} else if (name == "exp4") {
					continue;
				} else if (name == "exp_comp_g") {
					res = res > _eval(*exp4.value(), args);
				} else if (name == "exp_comp_ge") {
					res = res >= _eval(*exp4.value(), args);
				} else if (name == "exp_comp_l") {
					res = res < _eval(*exp4.value(), args);
				} else if (name == "exp_comp_le") {
					res = res <= _eval(*exp4.value(), args);
				} else {
					return KError::codegen("Unknown child in _eval_exp6: " + std::string(name));
				}
			}
			return res;
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_exp7(
		AstNode const &node,
		TemplDict const &args
	) {
		try {
			auto exp = node.child_with_cfg("exp6").value();
			auto res = _eval(*exp, args);

			for (auto &child : node) {
				//TODO
				//if (child.type() == AstNode::Type::Token) continue;
				auto name = child.cfg_rule();
				auto exp6 = child.child_with_cfg("exp6");
				if (name == "whitespace") {
					continue;
				} else if (name == "exp6") {
					continue;
				} else if (name == "exp_comp_eq") {
					res = res == _eval(*exp6.value(), args);
				} else if (name == "exp_comp_neq") {
					res = res != _eval(*exp6.value(), args);
				} else {
					return KError::codegen("Unknown child in _eval_exp7: " + std::string(name));
				}
			}
			return res;
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_exp11(
		AstNode const &node,
		TemplDict const &args
	) {
		try {
			auto exp = node.child_with_cfg("exp7").value();
			auto res = _eval(*exp, args);

			for (auto &child : node) {
				//TODO
				//if (child.type() == AstNode::Type::Token) continue;
				auto name = child.cfg_rule();
				auto exp7 = child.child_with_cfg("exp7");
				if (name == "whitespace") {
					continue;
				} else if (name == "exp7") {
					continue;
				} else if (name == "exp_log_and") {
					res = res && _eval(*exp7.value(), args);
				} else {
					return KError::codegen("Unknown child in _eval_exp11: " + std::string(name));
				}
			}
			return res;
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_exp12(
		AstNode const &node,
		TemplDict const &args
	) {
		try {
			auto exp = node.child_with_cfg("exp11").value();
			auto res = _eval(*exp, args);

			for (auto &child : node) {
				//TODO
				//if (child.type() == AstNode::Type::Token) continue;
				auto name = child.cfg_rule();

				auto exp11 = child.child_with_cfg("exp11");
				if (name == "whitespace") {
					continue;
				} else if (name == "exp11") {
					continue;
				} else if (name == "exp_log_or") {
					res = res || _eval(*exp11.value(), args);
				} else {
					if (name.empty()) name = child.tok().debug_str();
					return KError::codegen("Unknown child in _eval_exp12: " + std::string(name));
				}
			}
			return res;
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_filter_frag(
		TemplObj const &lhs,
		AstNode const &node,
		TemplDict const &args
	) {
		try {
			CG_ASSERT(node.cfg_rule() == "exp_filter_frag", "Must be an filter frag");
			auto filter = _eval_exp_id(*node.child_with_tok(Token::Type::Ident).value(), args);
			auto l_args = args;
			l_args["self"] = lhs;
			if (auto call = node.child_with_cfg("exp_call")) {
				return _eval_exp_call(filter.value(), *call.value(), l_args);
			} else {
				auto call_args = TemplList();
				call_args.push_back(lhs);
				return filter->func().value()(call_args);
			}
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_filter(
		AstNode const &node,
		TemplDict const &args
	) {
		try {
			auto exp = node.child_with_cfg("exp12").value();
			auto res = _eval(*exp, args);

			for (auto &child : node) {
				auto name = child.cfg_rule();
				if (name == "whitespace") {
					continue;
				} else if (name == "exp12") {
					continue;
				} else if (name == "exp_filter_frag") {
					res = _eval_filter_frag(res.value(), child, args);
				}
			}
			return res;
		} catch_kerror;
	}


	util::Result<bool, KError> TemplGen::_tag_keep_padding(
		AstNode const &node,
		bool def
	) {
		auto cons = node.consumed_all();
		auto const &name = node.cfg_rule();
		if (name == "comment_b" || name == "expression_b") {
			if (cons.size() == 2) {
				return def;
			}
			CG_ASSERT(cons.size() == 3, "Invalid beggining tag size");
			if (cons[2] == '-') {
				return false;
			}
			if (cons[2] == '+') {
				return true;
			}
			return KError::codegen(util::f("Invalid tag ending: ", cons[2]), node.location());
		} else if (name == "comment_e" || name == "expression_e") {
			if (cons.size() == 2) {
				return def;
			}
			CG_ASSERT(cons.size() == 3, "Invalid ending tag size");
			if (cons[0] == '-') {
				return false;
			}
			if (cons[0] == '+') {
				return true;
			}
			return KError::codegen(util::f("Invalid tag beggining: ", cons[0]), node.location());
		} else {
			return KError::codegen(util::f("Cannot get tag padding for cfg node ", name));
		}
	}

	util::Result<void, KError> TemplGen::_add_builtin_identifier(
		std::string const &name,
		TemplObj const &func,
		TemplDict &args
	) {
		if (args.contains(name)) {
			return KError::codegen(
				util::f("Cannot pass in arg with name ", name, " because it is a builtin identifier")
			);
		} else {
			args[name] = func;
		}
		return {};
	}

	TemplFuncRes _builtin_abs(TemplInt i) {
		return {std::abs(i)};
	}
	TemplFuncRes _builtin_capitilize(std::string s) {
		auto r = s;
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		if (r.size() > 0) {
			r[0] = ::toupper(r[0]);
		}
		return {r};
	}
	TemplFuncRes _builtin_center_int(TemplStr s, TemplInt i) {
		auto r = std::string();
		for (auto line : util::get_lines(s)) {
			auto padding = (i - line.size()) / 2;
			r += std::string(padding, ' ') + std::string(line);
		}
		return {r};
	}
	TemplFuncRes _builtin_center(TemplStr s) {
		return _builtin_center_int(s, 80);
	}
	TemplFuncRes _builtin_first(TemplList l) {
		if (l.size() == 0) {
			return KError::codegen("Cannot take first of list with size 0");
		}
		return {l[0]};
	}
	TemplFuncRes _builtin_indent(TemplList args) {
		try {
			if (args.size() < 1 || args.size() > 4) {
				return KError::codegen(util::f(args.size(), " is not a valid arg count for filter."));
			}
			auto r = std::string();
			auto str = args[0].str().value();
			auto indent_str = std::string("    ");
			auto indent = false;
			auto indent_blank = false;

			//Parse args

			// Indent Value
			if (args.size() >= 2) {
				if (auto str = args[1].str(false)) {
					indent_str = str.value();
				} else if (auto i = args[1].integer()) {
					indent_str = std::string(i.value(), ' ');
				};
			}

			// Indent first?
			if (args.size() >= 3) {
				// indent first?
				indent = args[2].boolean().value();
			}

			//Skip empty?
			if (args.size() >= 4) {
				indent_blank = args[3].boolean().value();
			}

			for (auto line : util::get_lines(str)) {
				bool is_blank = std::all_of(line.begin(), line.end(), isspace);

				if (indent) {
					if (!is_blank || indent_blank) {
						r += indent_str;
					}
				} else {
					indent = true;
				}
				r += std::string(line) + "\n";
			}

			return {r};
		} catch_kerror;
	}

	util::Result<void, KError> TemplGen::_add_builtin_identifiers(TemplDict &args) {
		TRY(_add_builtin_identifier("true", true, args));
		TRY(_add_builtin_identifier("false", false, args));

		TRY(_add_builtin_identifier("abs", mk_templfunc(_builtin_abs), args));
		TRY(_add_builtin_identifier("capitilize", mk_templfunc(_builtin_capitilize), args));
		TRY(_add_builtin_identifier(
				"center",
				mk_templfuncs(_builtin_center, _builtin_center_int),
				args
		));
		TRY(_add_builtin_identifier("first", mk_templfunc(_builtin_first), args));
		TRY(_add_builtin_identifier("indent", _builtin_indent, args));
		return {};
	}
}
