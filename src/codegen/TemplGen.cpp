#include "TemplGen.hpp"
#include "codegen/AstNode.hpp"
#include "codegen/CfgContext.hpp"
#include "codegen/SParser.hpp"
#include "codegen/TemplObj.hpp"
#include "util/IterAdapter.hpp"
#include "util/file.hpp"
#include "util/log.hpp"
#include "util/lines_iterator.hpp"
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

	util::Result<void, Error> TemplGen::_setup_parser() {
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

		if (auto err = c.prep().move_or()) {
			return Error(ErrorType::MISC, "Could not prepare parser", err.value());
		}
		c.simplify();

		if (true) {
			//std::ofstream file("gen/nothing-table.txt");
			if (auto err = AbsoluteSolver::create(std::move(context)).move_or(_parser)) {
				return Error(ErrorType::INTERNAL, "Could not initialize the AbsoluteSolver", *err);
			}
			//parser->print_table(file);
		} else {
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

	util::Result<std::string, Error> TemplGen::codegen(
		std::string const &str,
		TemplObj const &args,
		std::string const &filename
	) {
		if (auto dict = args.dict()) {
			return codegen(str, dict.value(), filename);
		} else {
			return Error(ErrorType::INTERNAL, "Args must be a dictionary");
		}
	}

	util::Result<std::string, Error> TemplGen::codegen(
		std::string const &str,
		TemplDict const &args,
		std::string const &filename
	) {
		if (auto err = _setup_parser().move_or()) {
			return Error(ErrorType::INTERNAL, "Could not setup parser", *err);
		}
		auto t = TemplGen();
		std::ofstream file("gen/templgen.gv");
		auto label = util::f("Graph for file: ", filename);

		AstNode *node;
		if (auto err = _parser->parse({str.c_str(), filename.c_str()}, t._parser_result).move_or(node)) {
			return Error(ErrorType::INVALID_PARSE, "Cannot parse AstNode", *err);
		}
		node->compress(_parser->cfg().prim_names());

		node->print_dot(file, label);

		file.close();

		auto l_args = args;
		if (auto err = t._add_builtin_identifiers(l_args).move_or()) {
			return Error(ErrorType::MISC, "Could not add builtin identifiers", err.value());
		}
		return t._codegen(*node, l_args);
	}

	util::Result<std::string, Error> _unpack_str(std::string const &str) {
		auto s = std::string();
		auto c = str.c_str();

		CG_ASSERT(*c == '"', "String literal must start with '\"'");
		c++;
		while (*c != '"') {
			if (*c == '\0') {
				return Error(ErrorType::INTERNAL, "Unexpected end to string sequence");
			}

			if (*c == '\\') {
				c++;
				switch (*c) {
					case '"':
						s += '"';
						break;
					default:
						return Error(ErrorType::ASSERT, util::f("Unknown string escape sequence: \\", *c));
				}
			} else {
				s += *c;
			}
			c++;
		}
		return s;
	}

	util::Result<std::string, Error> TemplGen::_codegen(
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
			if (auto err = _cg_smacro(node, args).move_or()) {
				log_error(err.value());
			}
			return {""};
		} else if (node.cfg_rule() == "sinclude") {
			return _cg_sinclude(node, args);
		} else if (node.cfg_rule() == "sfrag_endmacro") {
			return {""};
		} else if (node.cfg_rule() == "sfrag_endfor") {
			return {""};
		} else {
			return Error(ErrorType::INTERNAL, util::f("Unimplimented AstNode type: ", std::string(node.cfg_rule())));
		}
	}

	util::Result<std::string, Error> TemplGen::_cg_default(
		AstNode const &node,
		TemplDict &args
	) {
		return node.consumed_all();
	}

	TemplGen::CodegenRes TemplGen::_cg_recursive(
		AstNode const &node,
		TemplDict &args
	) {
		auto r = node.tok().content();
		for (auto &child : node) {
			if (auto r2 = _codegen(child, args)) {
				r += r2.value();
			} else {
				return r2.error();
			}
		}
		return r;
	}

	util::Result<std::string, Error> TemplGen::_cg_ref(
		AstNode const &node,
		TemplDict &args,
		size_t count
	) {
		//CG_ASSERT(node.children().size() == count, "Children count of codegen_ref must be 1");
		return _codegen(*node.begin(), args);
	}

	util::Result<std::string, Error> TemplGen::_cg_identifier(
		AstNode const &node,
		TemplDict &args
	) {
		return Error(ErrorType::INTERNAL, "Identifier does not have a codegen implimentation");
	}

	util::Result<std::string, Error> TemplGen::_cg_line(
		AstNode const &node,
		TemplDict &args
	) {
		auto result = std::string();
		for (auto &child : node) {
			if (auto str = _codegen(child, args)) {
				result += str.value();
			} else {
				return str.error();
			}
		}
		return result;
	}

	util::Result<std::string, Error> TemplGen::_cg_lines(
		AstNode const &node,
		TemplDict &args
	) {
		auto result = std::string();
		for (auto &child : node) {
			if (auto str = _codegen(child, args)) {
				result += str.value();
			} else {
				return str.error();
			}
		}
		return result;
	}

	util::Result<std::string, Error> TemplGen::_cg_comment(
		AstNode const &node,
		TemplDict &args
	) {
		return {""};
	}

	util::Result<std::string, Error> TemplGen::_cg_expression(
		AstNode const &node,
		TemplDict &args
	) {
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
				if (auto obj = _eval(child, args)) {
					if (auto str = obj->str()) {
						result += str.value();
					} else {
						return Error(ErrorType::SEMANTIC, "TemplObj is not a str", str.error());
					}
				} else {
					return Error(ErrorType::MISC, "Couldn't eval node", obj.error());
				}
			}
		}
		return result;
	}

	util::Result<std::string, Error> TemplGen::_cg_statement(
		AstNode const &node,
		TemplDict &args
	) {
		return Error(ErrorType::ASSERT, "Statement not implimented");
	}

	util::Result<std::string, Error> TemplGen::_cg_sif(
		AstNode const &node,
		TemplDict &args
	) {
		auto result = std::string();
		CG_ASSERT(node.cfg_rule() == "sif", "INTERNAL func can only parser sif nodes");

		cg::AstNode *sif_chain;
		if (auto n = node.child_with_cfg("sif_start_chain").move_or(sif_chain)) {
			return Error(ErrorType::ASSERT, "Could not find sif_start_chain");
		}

		int i = 0;
		bool cg_current_block = false;
		TemplDict block_args;
		//TODO
		while (i < sif_chain->child_count()) {
			auto &child = sif_chain->begin()[i];
			if (cg_current_block) {
				if (child.cfg_rule() == "line_single") {
					if (auto s = _codegen(child, block_args)) {
						result += s.value();
					} else {
						return Error(ErrorType::MISC, "Error parsing block in if statement", s.error());
					}
				} else {
					break;
				}
			} else if (child.cfg_rule() == "sfrag_if") {
				cg::AstNode *exp_node;
				cg::TemplObj bool_value;
				bool raw_bool;

				if (auto err = child.child_with_cfg("exp").move_or(exp_node)) {
					return Error(ErrorType::ASSERT, "Expexted expression in if statement", err.value());
				}
				if (auto err = _eval(*exp_node, args).move_or(bool_value)) {
					return Error(ErrorType::SEMANTIC, "Expected expression to resolve in a boolean", err.value());
				}
				if (auto err = bool_value.boolean().move_or(raw_bool)) {
					return Error(ErrorType::SEMANTIC, "Exp in if statement did not resolve in a bool");
				}
				if (raw_bool) {
					block_args = args;
					cg_current_block = true;
				}
			} else if (child.cfg_rule() == "sfrag_elif") {
				cg::AstNode *exp_node;
				cg::TemplObj bool_value;
				bool raw_bool;

				if (auto err = child.child_with_cfg("exp").move_or(exp_node)) {
					return Error(ErrorType::MISC, "Error parsing exp in elif statement", err.value());
				}
				if (auto err = _eval(*exp_node, args).move_or(bool_value)) {
					return Error(ErrorType::MISC, "Error parsing bool in elif statement", err.value());
				}
				if (auto err = bool_value.boolean().move_or(raw_bool)) {
					return Error(ErrorType::SEMANTIC, "Exp in elif statement did no resolve in a bool");
				}
				if (raw_bool) {
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
	}

	util::Result<std::string, Error> TemplGen::_cg_sfor(
		AstNode const &node,
		TemplDict &args
	) {
		auto result = std::string();
		AstNode *sfrag_for, *identifier, *iter_exp, *lines;
		std::string iter_name;

		if (auto err = node.child_with_cfg("sfrag_for").move_or(sfrag_for)) {
			return Error(ErrorType::MISC, "Could not parse for statement", err.value());
		}
		if (auto err = sfrag_for->child_with_tok(Token::Type::Ident).move_or(identifier)) {
			return Error(ErrorType::ASSERT, "Could not parse identifier statement in for statement", err.value());
		}
		if (auto err = sfrag_for->child_with_cfg("exp").move_or(iter_exp)) {
			return Error(ErrorType::ASSERT, "Could not parse for loop iterator expression", err.value());
		}
		if (auto err = node.child_with_cfg("sfor_lines").move_or(lines)) {
			return Error(ErrorType::ASSERT, "Could not parse lines in for statement", err.value());
		}

		iter_name = identifier->tok().content();


		auto iter_obj = TemplObj();
		if (auto err = _eval(*iter_exp, args).move_or(iter_obj)) {
			return Error(ErrorType::MISC, "Could not parse iter obj", err.value());
		}
		if (iter_obj.type() != TemplObj::Type::List) {
			return Error(ErrorType::RUNTIME_CG, "For statement expression must be a list", iter_exp->location());
		}
		int index = 0;
		auto l = iter_obj.list().value();
		for (auto &i : l) {
			auto loop = TemplObj{
				{"index", index+1},
				{"index0", index},
				{"first", index==0},
				{"last", index==iter_obj.list().value().size()-1}
			};

			auto local_args = args;
			local_args[iter_name] = i;
			local_args["loop"] = loop;
			if (auto v = _codegen(*lines, local_args)) {
				result += v.value();
			} else {
				return Error(ErrorType::MISC, "Could not evaluate loop in for loop", v.error());
			}
			index++;
		}

		return result;
	}

	util::Result<void, Error> TemplGen::_cg_smacro(
		AstNode const &node,
		TemplDict &args
	) {
		AstNode *arg_def, *content, *ident;
		std::string macro_name;

		if (auto err = node.child_with_cfg("sfrag_macro").move_or(arg_def)) {
			return Error(ErrorType::ASSERT, "Could not parse sfrag_macro", err.value());
		}
		if (auto err = arg_def->child_with_tok(Token::Type::Ident).move_or(ident)) {
			return Error(ErrorType::ASSERT, "Could not find identifier token", err.value());
		}
		auto macro_arg_list = arg_def->child_with_cfg("sfrag_argdef_list").value(nullptr);
		if (auto err = node.child_with_cfg("smacro_lines").move_or(content)) {
			return Error(ErrorType::ASSERT, "Could not find smacro_lines", err.value());
		}
		macro_name = ident->tok().content();

		if (args.contains(macro_name)) {
			return Error(ErrorType::SEMANTIC, util::f(
				"Cannot create macro with name ",
				macro_name,
				" because identifier already exists."
			));
		}

		auto macro_args = std::vector<std::tuple<std::string, TemplObj>>();
		if (macro_arg_list) {
			for (auto &macro_arg_node : macro_arg_list->children_with_cfg("sfrag_argdef")) {
				AstNode *arg_name;
				auto macro_arg_value = TemplObj();

				if (auto err = macro_arg_node->child_with_tok(Token::Type::Ident).move_or(arg_name)) {
					return Error(ErrorType::ASSERT, "Cannot find identifier in macro arg", err.value());
				}
				if (auto exp_node = macro_arg_node->child_with_cfg("exp")) {
					if (auto err = _eval(*exp_node.value(), args).move_or(macro_arg_value)) {
						return Error(ErrorType::RUNTIME_CG, "Could not evaluate macro default argument", err.value());
					}
				}

				macro_args.push_back({arg_name->tok().content(), macro_arg_value});
			}
		}
		
		TemplFunc func = [this, macro_args, args, macro_name, content](TemplList l) -> TemplFuncRes {
			auto local_args = args;
			if (macro_args.size() < l.size()) {
				return Error(ErrorType::SEMANTIC, util::f(
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
						return Error(ErrorType::SEMANTIC, util::f(
							"Not enough arguments passed to macro:",
							macro_name
						));
					} else {
						local_args[arg_name] = arg_value;
					}
				}
				i++;
			}
			if (auto str = _codegen(*content, local_args)) {
				return {str.value()};
			} else {
				return {Error(ErrorType::MISC, "Problem evaluating content of macro", str.error())};
			}
		};
		args[macro_name] = func;
		return {};
	}

	TemplGen::CodegenRes TemplGen::_cg_sinclude(
		AstNode const &node,
		TemplDict &args
	) {
		AstNode *file_url_node, *included_node;
		std::string filename;

		if (auto err = node.child_with_tok(Token::Type::StrConst).move_or(file_url_node)) {
			return Error(ErrorType::ASSERT, "Could not filename node", err.value());
		}
		if (auto err =_unpack_str(file_url_node->tok().content()).move_or(filename)) {
			return Error(ErrorType::ASSERT, "Could not unpack filename string", err.value());
		}
		auto include_src = util::readEnvFile(filename);
		if (auto err = _parser->parse({include_src.c_str(), filename.c_str()}, _parser_result).move_or(included_node)) {
			return Error(ErrorType::MISC, util::f("Error in included file ", filename), err.value());
		}
		included_node->compress(_parser->cfg().prim_names());


		std::ofstream file("gen/templgen-include.gv");
		included_node->print_dot(file, "templgen-include");
		file.close();

		return _codegen(*included_node, args);
	}

	TemplGen::EvalRes TemplGen::_eval(
		util::Result<AstNode, Error> const &node,
		TemplDict const &args
	) {
		if (node.has_value()) {
			return _eval(node.value(), args);
		} else {
			return Error(ErrorType::MISC, "Could not eval node", node.error());
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
			return Error(ErrorType::INTERNAL, util::f("Unimplimented AstNode type ", node.cfg_rule()));
		}
	}

	TemplGen::EvalRes TemplGen::_eval_exp_sing(
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
				return Error(ErrorType::INTERNAL, util::f("Unknown node passed to _eval_exp_sing: ", child.str()));
			}
		}
		return Error(ErrorType::INTERNAL, "exp_sing is an empty node");
	}

	TemplGen::EvalRes TemplGen::_eval_exp_id(
		AstNode const &node,
		TemplDict const &args
	) {
		auto name = node.tok().content();
		if (args.count(name) == 0) {
			return Error(ErrorType::SEMANTIC, util::f("Unknown identifier \"", name, "\" ", node.location()));
		}
		return args.at(name).dup().set_location(node.location());
	}
	
	TemplGen::EvalRes TemplGen::_eval_exp_int(
		AstNode const &node,
		TemplDict const &args
	) {
		CG_ASSERT(node.tok().type() == Token::Type::IntConst, "IntConst must be passed to _eval_exp_int");
		auto value = TemplInt(0);
		for (auto c : node.consumed_all()) {
			value = value * 10 + c - '0';
		}
		auto value_obj = TemplObj(value).set_location(node.location());
		return {value_obj};
	}

	TemplGen::EvalRes TemplGen::_eval_exp_str(
		AstNode const &node,
		TemplDict const &args
	) {
		if (auto str = _unpack_str(node.consumed_all())) {
			return TemplObj(str.value()).set_location(node.location());
		} else {
			return str.error();
		}
	}

	TemplGen::EvalRes TemplGen::_eval_exp1(
		AstNode const &node,
		TemplDict const &args
	) {
		TemplDict l_args = args;
		AstNode *exp_node;
		TemplObj res;

		if (auto err = node.child_with_cfg("exp_sing").move_or(exp_node)) {
			return Error(ErrorType::ASSERT, "Expression does not have exp_sing child", err.value());
		}
		if (auto err = _eval(*exp_node, l_args).move_or(res)) {
			return Error(ErrorType::MISC, "Could not eval expression", err.value());
		}
		CG_ASSERT(node.begin() != node.end(), "Node must have at least one child");

		auto start = node.begin();
		start++;
		auto children = util::Adapt(start, node.end());
		for (auto const &child : children) {
			//TODO
			//if (child.type() == AstNode::Type::Token) continue;
			if (child.cfg_rule() == "exp_member") {
				l_args["self"] = res;
				if (auto err = _eval_exp_member(res, child, l_args).move_or(res)) {
					return Error(ErrorType::MISC, "Could not evaluate property.", err.value());
				}
			} else if (child.cfg_rule() == "exp_call") {
				if (auto err = _eval_exp_call(res, child, l_args).move_or(res)) {
					return Error(ErrorType::MISC, "Could not call function.", err.value());
				}
			} else {
				return Error(ErrorType::INTERNAL, util::f("Unrecognized cfg node: ", child.cfg_rule()));
			}
		}
		return res;
	}

	TemplGen::EvalRes TemplGen::_eval_exp_member(
		TemplObj const &lhs,
		AstNode const &node,
		TemplDict const &args
	) {
		AstNode *ident_node;
		if (auto err = node.child_with_tok(Token::Type::Ident).move_or(ident_node)) {
			return Error(ErrorType::ASSERT, "Could not find member identifier", err.value());
		}
		return lhs.get_attribute(ident_node->tok().content());
	}

	TemplGen::EvalRes TemplGen::_eval_exp_call(
		TemplObj const &lhs,
		AstNode const &node,
		TemplDict const &args
	) {
		auto call_args = TemplList();
		if (args.contains("self")) {
			call_args.push_back(args.at("self"));
		}
		for (auto const &child : node) {
			TemplObj child_val;
			if (child.type() == AstNode::Type::Leaf) continue;
			if (child.cfg_rule() == "whitespace") continue;
			CG_ASSERT(child.cfg_rule() == "exp", "Function call list must have exp nodes");
			if (auto err = _eval(child, args).move_or(child_val)) {
				return Error(ErrorType::MISC, "Could not evaluate expression in one of the arguments", err.value());
			}
			call_args.push_back(child_val);
		}

		if (lhs.type() != TemplObj::Type::Func) {
			return Error(ErrorType::SEMANTIC, "Trying to call with an object that is not a function.");
		}
		return lhs.func().value()(call_args);
	}

	TemplGen::EvalRes TemplGen::_eval_exp2(
		AstNode const &node,
		TemplDict const &args
	) {
		for (auto &child : node) {
			auto name = child.cfg_rule();
			AstNode *exp2;

			if (name == "whitespace") {
				continue;
			} else if (name == "exp1") {
				return _eval(child, args);
			} else {
				if (auto err = child.child_with_cfg("exp2").move_or(exp2)) {
					return Error(ErrorType::ASSERT, "Expecting an exp2 node", err.value());
				}
				if (name == "exp_plus") {
					return +_eval(*exp2, args);
				} else if (name == "exp_min") {
					return -_eval(*exp2, args);
				} else if (name == "exp_log_not") {
					return !_eval(*exp2, args);
				} else {
					return Error(ErrorType::ASSERT, util::f("Unknown child in _eval_exp2: ", name));
				}
		}
		}
		return Error(ErrorType::ASSERT, "Empty node passed to _eval_exp2");
	}

	TemplGen::EvalRes TemplGen::_eval_exp3(
		AstNode const &node,
		TemplDict const &args
	) {
		AstNode *exp_node, *exp2_node;
		TemplGen::EvalRes res = TemplObj();

		if (auto err = node.child_with_cfg("exp2").move_or(exp_node)) {
			return Error(ErrorType::ASSERT, "Expecting an exp2 node");
		}
		if (auto err = _eval(*exp_node, args).move_or(res)) {
			return Error(ErrorType::MISC, "Cannot evaluate exp2", err.value());
		}

		for (auto &child : node) {
			auto name = child.cfg_rule();

			if (name == "whitespace") {
				continue;
			} else if (name == "exp2") {
				continue;
			} else {
				if (auto err = child.child_with_cfg("exp2").move_or(exp2_node)) {
					return Error(ErrorType::MISC, "Cannot evaluate exp2", err.value());
				}

				if (name == "exp_mult") {
					res = res * _eval(*exp2_node, args);
				} else if (name == "exp_div") {
					res = res / _eval(*exp2_node, args);
				} else if (name == "exp_mod") {
					res = res % _eval(*exp2_node, args);
				} else {
					return Error(ErrorType::MISC, util::f("Unknown child in _eval_exp3: ", name));
				}
			}
		}
		return res;
	}

	TemplGen::EvalRes TemplGen::_eval_exp4(
		AstNode const &node,
		TemplDict const &args
	) {
		AstNode *exp_node;
		TemplGen::EvalRes res = TemplObj();

		if (auto err = node.child_with_cfg("exp3").move_or(exp_node)) {
			return Error(ErrorType::ASSERT, "No child node exp3", err.value());
		}
		if (auto err = _eval(*exp_node, args).move_or(res)) {
			return Error(ErrorType::MISC, "Cannot evaluate exp3", err.value());
		}

		for (auto &child : node) {
			AstNode *exp3_node;

			auto name = child.cfg_rule();

			if (name == "whitespace" || name == "exp3") continue;

			if (auto err = child.child_with_cfg("exp3").move_or(exp3_node)) {
				return Error(ErrorType::ASSERT, "child is not an exp3", err.value());
			}

			if (name == "exp_add") {
				res = res + _eval(*exp3_node, args);
			} else if (name == "exp_sub") {
				res = res - _eval(*exp3_node, args);
			} else {
				return Error(ErrorType::INTERNAL, util::f("Unknown child in _eval_exp4: ", name));
			}
		}
		return res;
	}

	TemplGen::EvalRes TemplGen::_eval_exp6(
		AstNode const &node,
		TemplDict const &args
	) {
		AstNode *exp_node;
		TemplGen::EvalRes res = TemplObj();

		if (auto err = node.child_with_cfg("exp4").move_or(exp_node)) {
			return Error(ErrorType::ASSERT, "Expecting exp4 child", err.value());
		}
		if (auto err = _eval(*exp_node, args).move_or(res)) {
			return Error(ErrorType::MISC, "Could not evaluate exp4", err.value());
		}

		for (auto &child : node) {
			AstNode *exp4_node;
			auto name = child.cfg_rule();

			if (name == "whitespace" || name == "exp4") continue;

			if (auto err = child.child_with_cfg("exp4").move_or(exp4_node)) {
				return Error(ErrorType::ASSERT, "Expecting exp4 child", err.value());
			}

			if (name == "exp_comp_g") {
				res = res > _eval(*exp4_node, args);
			} else if (name == "exp_comp_ge") {
				res = res >= _eval(*exp4_node, args);
			} else if (name == "exp_comp_l") {
				res = res < _eval(*exp4_node, args);
			} else if (name == "exp_comp_le") {
				res = res <= _eval(*exp4_node, args);
			} else {
				return Error(ErrorType::INTERNAL, util::f("Unknown child in _eval_exp6: ", name));
			}
		}
		return res;
	}

	TemplGen::EvalRes TemplGen::_eval_exp7(
		AstNode const &node,
		TemplDict const &args
	) {
		AstNode *exp_node;
		TemplGen::EvalRes res = TemplObj();

		if (auto err = node.child_with_cfg("exp6").move_or(exp_node)) {
			return Error(ErrorType::ASSERT, "Expecting exp6 child node", err.value());
		}
		if (auto err = _eval(*exp_node, args).move_or(res)) {
			return Error(ErrorType::MISC, "Could not evaluate exp6", err.value());
		}

		for (auto &child : node) {
			AstNode *exp6_node;

			auto name = child.cfg_rule();

			if (name == "whitespace" || name == "exp6") continue;

			if (auto err = child.child_with_cfg("exp6").move_or(exp6_node)) {
				return Error(ErrorType::ASSERT, "Expecting exp6 child node", err.value());
			}

			if (name == "exp_comp_eq") {
				res = res == _eval(*exp6_node, args);
			} else if (name == "exp_comp_neq") {
				res = res != _eval(*exp6_node, args);
			} else {
				return Error(ErrorType::ASSERT, util::f("Unknown child in _eval_exp7: ", name));
			}
		}
		return res;
	}

	TemplGen::EvalRes TemplGen::_eval_exp11(
		AstNode const &node,
		TemplDict const &args
	) {
		AstNode *exp_node;
		TemplGen::EvalRes res = TemplObj();
		if (auto err = node.child_with_cfg("exp7").move_or(exp_node)) {
			return Error(ErrorType::ASSERT, "Expecting exp7 child node", err.value());
		}
		if (auto err = _eval(*exp_node, args).move_or(res)) {
			return Error(ErrorType::MISC, "Could not evaluate the exp7 node", err.value());
		}

		for (auto &child : node) {
			AstNode *exp7_node;
			auto name = child.cfg_rule();

			if (name == "whitespace" || name == "exp7") continue;

			if (auto err = child.child_with_cfg("exp7").move_or(exp7_node)) {
				return Error(ErrorType::ASSERT, "Expecting exp7 child node", err.value());
			}

			if (name == "exp_log_and") {
				res = res && _eval(*exp7_node, args);
			} else {
				return Error(ErrorType::ASSERT, util::f("Unknown child in _eval_exp11", name));
			}
		}
		return res;
	}

	TemplGen::EvalRes TemplGen::_eval_exp12(
		AstNode const &node,
		TemplDict const &args
	) {
		AstNode *exp_node;
		TemplGen::EvalRes res = TemplObj();

		if (auto err = node.child_with_cfg("exp11").move_or(exp_node)) {
			return Error(ErrorType::ASSERT, "Expecting exp11 child node", err.value());
		}
		if (auto err = _eval(*exp_node, args).move_or(res)) {
			return Error(ErrorType::MISC, "Could not evaluate the exp11 node", err.value());
		}

		for (auto &child : node) {
			AstNode *temp_exp_node;
			auto name = child.cfg_rule();

			if (name == "whitespace" || name == "exp11") continue;

			if (auto err = child.child_with_cfg("exp11").move_or(temp_exp_node)) {
				return Error(ErrorType::ASSERT, "Expecting exp11 child node", err.value());
			}

			if (name == "exp_log_or") {
				res = res || _eval(*temp_exp_node, args);
			} else {
				if (name.empty()) name = child.tok().debug_str();
				return Error(ErrorType::ASSERT, util::f("Unknown child in _eval_exp12: ", name));
			}
		}
		return res;
	}

	TemplGen::EvalRes TemplGen::_eval_filter_frag(
		TemplObj const &lhs,
		AstNode const &node,
		TemplDict const &args
	) {
		AstNode *filter_node;
		TemplObj filter_func;
		CG_ASSERT(node.cfg_rule() == "exp_filter_frag", "Must be an filter frag");
		
		if (auto err = node.child_with_tok(Token::Type::Ident).move_or(filter_node)) {
			return Error(ErrorType::ASSERT, "Can't find name of the filter", err.value());
		}
		if (auto err = _eval_exp_id(*filter_node, args).move_or(filter_func)) {
			return Error(ErrorType::MISC, "Cannot evaluate filter identifier", err.value());
		}
		auto l_args = args;
		l_args["self"] = lhs;
		if (auto call = node.child_with_cfg("exp_call")) {
			if (auto r = _eval_exp_call(filter_func, *call.value(), l_args)) {
				return r;
			} else {
				return Error(ErrorType::RUNTIME_CG, util::f("Could not call filter: '", node.str_src(), "'"));
			}
		} else {
			auto call_args = TemplList();
			call_args.push_back(lhs);
			CG_ASSERT(filter_func.type() == TemplObj::Type::Func, "Filter identifier must by a function");
			return filter_func.func().value()(call_args);
		}
	}

	TemplGen::EvalRes TemplGen::_eval_filter(
		AstNode const &node,
		TemplDict const &args
	) {
		AstNode *exp_node;
		TemplGen::EvalRes res = TemplObj();
		if (auto err = node.child_with_cfg("exp12").move_or(exp_node)) {
			return Error(ErrorType::ASSERT, "Cannot find expression", err.value());
		}
		if (auto err = _eval(*exp_node, args).move_or(res)) {
			return Error(ErrorType::MISC, "Could not evaluate exp12", err.value());
		}

		for (auto &child : node) {
			auto name = child.cfg_rule();
			if (name == "whitespace") {
				continue;
			} else if (name == "exp12") {
				continue;
			} else if (name == "exp_filter_frag") {
				if (res) {
					res = _eval_filter_frag(res.value(), child, args);
				} else {
					return res.error();
				}
			}
		}
		return res;
	}


	util::Result<bool, Error> TemplGen::_tag_keep_padding(
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
			return Error(ErrorType::SEMANTIC, util::f("Invalid tag ending: ", cons[2]), node.location());
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
			return Error(ErrorType::SEMANTIC, util::f("Invalid tag beggining: ", cons[0]), node.location());
		} else {
			return Error(ErrorType::ASSERT, util::f("Cannot get tag padding for cfg node", name));
		}
	}

	util::Result<void, Error> TemplGen::_add_builtin_identifier(
		std::string const &name,
		TemplObj const &func,
		TemplDict &args
	) {
		if (args.contains(name)) {
			return Error(
				ErrorType::SEMANTIC,
				util::f("Cannot pass in arg with name ", name, " because it is a builting identifier")
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
			return Error(ErrorType::RUNTIME_CG, "Cannot take first of list with size 0");
		}
		return {l[0]};
	}
	TemplFuncRes _builtin_indent(TemplList args) {
		if (args.size() < 1 || args.size() > 4) {
			return Error(ErrorType::RUNTIME_CG, util::f(args.size(), " is not a valid count for filter."));
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
			if (auto b = args[2].boolean()) {
				indent = b.value();
			} else {
				return Error(ErrorType::RUNTIME_CG, "Expecting a boolean for third argument in builtin indent");
			}
		}

		//Skip empty?
		if (args.size() >= 4) {
			if (auto b = args[3].boolean()) {
				indent_blank = b.value();
			} else {
				return Error(ErrorType::RUNTIME_CG, "Expecting a boolean for the third argument in builtin index");
			}
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
	}

	util::Result<void, Error> TemplGen::_add_builtin_identifiers(TemplDict &args) {
		if (auto err = _add_builtin_identifier("true", true, args).move_or()) {
			return Error(ErrorType::ASSERT, "Could not add true constant", err.value());
		}
		if (auto err = _add_builtin_identifier("false", false, args).move_or()) {
			return Error(ErrorType::ASSERT, "Could not add false constant", err.value());
		}

		if (auto err = _add_builtin_identifier("abs", mk_templfunc(_builtin_abs), args).move_or()) {
			return Error(ErrorType::ASSERT, "Could not add abs builtin function", err.value());
		}
		if (auto err = _add_builtin_identifier("capitilize", mk_templfunc(_builtin_capitilize), args).move_or()) {
			return Error(ErrorType::ASSERT, "Could not add builtin capitilize function", err.value());
		}
		if (auto err = _add_builtin_identifier(
			"center",
			mk_templfuncs(_builtin_center, _builtin_center_int),
			args
		).move_or()) {
			return Error(ErrorType::ASSERT, "Could not add builtin center function", err.value());
		}
		if (auto err = _add_builtin_identifier("first", mk_templfunc(_builtin_first), args).move_or()) {
			return Error(ErrorType::ASSERT, "Could not add builtin first function", err.value());
		}
		if (auto err = _add_builtin_identifier("indent", TemplFunc(_builtin_indent), args).move_or()) {
			return Error(ErrorType::ASSERT, "Could not add builting indent function", err.value());
		}

		return {};
	}
}
