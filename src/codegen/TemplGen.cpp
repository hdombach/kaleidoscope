#include "TemplGen.hpp"
#include "codegen/SParser.hpp"
#include "codegen/TemplObj.hpp"
#include "util/KError.hpp"
#include "util/IterAdapter.hpp"
#include "util/file.hpp"
#include "util/log.hpp"
#include "util/lines_iterator.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace cg {
	util::Result<TemplGen, KError> TemplGen::create() {
		auto result = TemplGen();
		auto &c = result._ctx;

		c.prim("whitespace") = c.cls(" "_cfg | "\t"_cfg | "\n"_cfg);
		c.prim("padding") = c.cls(" "_cfg | "\t"_cfg);
		c.temp("digit") =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;
		c.temp("lower") =
			"a"_cfg | "b"_cfg | "c"_cfg | "d"_cfg | "e"_cfg | "f"_cfg | "g"_cfg | "h"_cfg |
			"i"_cfg | "j"_cfg | "k"_cfg | "l"_cfg | "m"_cfg | "n"_cfg | "o"_cfg | "p"_cfg |
			"q"_cfg | "r"_cfg | "s"_cfg | "t"_cfg | "u"_cfg | "v"_cfg | "w"_cfg | "x"_cfg |
			"y"_cfg | "z"_cfg;
		c.temp("upper") =
			"A"_cfg | "B"_cfg | "C"_cfg | "D"_cfg | "E"_cfg | "F"_cfg | "G"_cfg | "H"_cfg |
			"I"_cfg | "J"_cfg | "K"_cfg | "L"_cfg | "M"_cfg | "N"_cfg | "O"_cfg | "P"_cfg |
			"Q"_cfg | "R"_cfg | "S"_cfg | "T"_cfg | "U"_cfg | "V"_cfg | "W"_cfg | "X"_cfg |
			"Y"_cfg | "Z"_cfg;
		c.temp("alpha") = c["lower"] | c["uppoer"];
		c.temp("alnum") = c["alpha"] | c["digit"];
		c.prim("identifier") = ("_"_cfg | c["alpha"]) + c.cls(c["alnum"] | "_"_cfg);

		c.prim("padding_b") = c["padding"];
		c.prim("padding_e") = c["padding"];
		c.prim("padding_nl") = c["padding"] + c.opt("\n"_cfg);

		c.prim("raw") = c.cls(!(
			c["expression_b"] |
			c["statement_b"] |
			c["comment_b"] |
			"\n"_cfg
		));
		c.prim("line") = c.cls(c["statement"] | c["expression"] | c["comment"] | c["raw"]) + c.opt("\n"_cfg);
		c.prim("lines") = c.cls(c["line"]);
		c.prim("file") = c["lines"];

		c.prim("comment_b") = "{#"_cfg + c.opt("-"_cfg | "+"_cfg);
		c.prim("comment_e") = c.opt("-"_cfg | "+"_cfg) + "#}"_cfg;
		c.prim("comment") =
			c["padding_b"] +
			c["comment_b"] + c.cls(!"#}"_cfg) + c["comment_e"] +
			c["padding_e"];

		c.prim("expression_b") = "{{"_cfg + c.opt("-"_cfg | "+"_cfg);
		c.prim("expression_e") = c.opt("-"_cfg | "+"_cfg) + "}}"_cfg;
		c.prim("expression") =
			c["padding_b"] + c["expression_b"] +
			c["exp"] +
			c["expression_e"] + c["padding_e"];

		c.prim("exp_sing") = c["whitespace"] + (c["exp_id"] | c["exp_int"] | c["exp_str"] | c["exp_paran"]) + c["whitespace"];
		c.prim("exp_id") = c["whitespace"] + c["identifier"] + c["whitespace"];
		c.prim("exp_int") = c["digit"] + c.cls(c["digit"]);
		c.prim("exp_str") = "\""_cfg + c.cls(!("\""_cfg | "\\"_cfg) | "\\\""_cfg) + "\""_cfg;
		c.temp("exp_paran") = "("_cfg + c["exp"] + ")"_cfg;

		c.prim("exp1") = c["exp_sing"] + c.cls(c["exp_member"] | c["exp_call"]);
		c.prim("exp_member") = "."_cfg + c["exp_id"];
		c.prim("exp_call") = "("_cfg + c.opt(c["exp"] + c.cls(","_cfg + c["exp"])) + ")"_cfg;

		c.prim("exp2") = c["whitespace"] + (
			c["exp_plus"] |
			c["exp_min"] |
			c["exp_log_not"] |
			c["exp1"]
		) + c["whitespace"];

		c.prim("exp_plus") = "+"_cfg + c["exp2"];
		c.prim("exp_min") = "-"_cfg + c["exp2"];
		c.prim("exp_log_not") = "!"_cfg + c["exp2"];

		c.prim("exp3") = c["whitespace"] + c["exp2"] + c.cls(
			c["exp_mult"] |
			c["exp_div"] |
			c["exp_mod"]
		) + c["whitespace"];
		c.prim("exp_mult") = "*"_cfg + c["exp2"];
		c.prim("exp_div")  = "/"_cfg + c["exp2"];
		c.prim("exp_mod")  = "%"_cfg + c["exp2"];


		c.prim("exp4") = c["whitespace"] + c["exp3"] + c.cls(
			c["exp_add"] |
			c["exp_sub"]
		) + c["whitespace"];
		c.prim("exp_add") = "+"_cfg + c["exp3"];
		c.prim("exp_sub") = "-"_cfg + c["exp3"];

		c.prim("exp6") = c["whitespace"] + c["exp4"] + c.cls(
			c["exp_comp_g"] |
			c["exp_comp_ge"] |
			c["exp_comp_l"] |
			c["exp_comp_le"]
		) + c["whitespace"];
		c.prim("exp_comp_g") = ">"_cfg + c["exp4"];
		c.prim("exp_comp_ge") = ">="_cfg + c["exp4"];
		c.prim("exp_comp_l") = "<"_cfg + c["exp4"];
		c.prim("exp_comp_le") = "<="_cfg + c["exp4"];

		c.prim("exp7") = c["whitespace"] + c["exp6"] + c.cls(
			c["exp_comp_eq"] |
			c["exp_comp_neq"]
		) + c["whitespace"];
		c.prim("exp_comp_eq") = "=="_cfg + c["exp6"];
		c.prim("exp_comp_neq") = "!="_cfg + c["exp6"];

		c.prim("exp11") = c["whitespace"] + c["exp7"] + c.cls(
			c["exp_log_and"]
		) + c["whitespace"];
		c.prim("exp_log_and") = "&&"_cfg + c["exp7"];

		c.prim("exp12") = c["whitespace"] + c["exp11"] + c.cls(
			c["exp_log_or"]
		) + c["whitespace"];
		c.prim("exp_log_or") = "||"_cfg + c["exp11"];

		c.prim("exp_filter_frag") = c["exp_id"] + c.opt(c["exp_call"]);
		c.prim("exp_filter") = c["whitespace"] +
			c["exp12"] + c.cls("|"_cfg + c["exp_filter_frag"]) +
			c["whitespace"];

		c.prim("exp") = c["exp_filter"];

		c.prim("statement_b") = "{%"_cfg + c.opt("-"_cfg | "+"_cfg);
		c.prim("statement_e") = c.opt("-"_cfg | "+"_cfg) + "%}"_cfg;

		c.prim("sfrag_else") =
			c["padding_b"] + c["statement_b"] +
			c["whitespace"] + "else"_cfg + c["whitespace"] +
			c["statement_e"] + c["padding_nl"];

		c.prim("sfrag_if") =
			c["padding_b"] + c["statement_b"] +
			c["whitespace"] + "if"_cfg + c["whitespace"] + c["exp"] + c["whitespace"] +
			c["statement_e"] + c["padding_nl"];
		c.prim("sfrag_elif") =
			c["padding_b"] + c["statement_b"] +
			c["whitespace"] + "elif"_cfg + c["whitespace"] + c["exp"] + c["whitespace"] +
			c["statement_e"] + c["padding_nl"];
		c.temp("sfrag_endif") =
			c["padding_b"] + c["statement_b"] +
			c["whitespace"] + "endif"_cfg + c["whitespace"] +
			c["statement_e"] + c["padding_nl"];
		c.prim("sif_start_chain") = c["sfrag_if"] + c["lines"];
		c.prim("sif_elif_chain") = c["sfrag_elif"] + c["lines"];
		c.prim("sif_else_chain") = c["sfrag_else"] + c["lines"];
		c.prim("sif") =
			c["sif_start_chain"] +
			c.cls(c["sif_elif_chain"]) +
			c.opt(c["sif_else_chain"]) +
			c["sfrag_endif"];

		c.prim("sfrag_for") =
			c["padding_b"] + c["statement_b"] +
			c["padding"] + "for"_cfg + c["exp_id"] + "in"_cfg + c["exp"] +
			c["statement_e"] + c["padding_nl"];
		c.temp("sfrag_endfor") =
			c["padding_b"] + c["statement_b"] +
			c["whitespace"] + "endfor"_cfg +  c["whitespace"] +
			c["statement_e"] + c["padding_nl"];
		c.prim("sfor") = c["sfrag_for"] + c["lines"] + c["sfrag_endfor"];

		c.prim("sfrag_argdef_list") =
			c.opt(c["sfrag_argdef"] + c.cls(","_cfg + c["sfrag_argdef"]));
		c.prim("sfrag_argdef") =
			c["padding"] + c["identifier"] + c["padding"] +
			c.opt("="_cfg + c["exp"]);

		c.prim("sfrag_macro") =
			c["padding_b"] + c["statement_b"] +
			c["padding"] + "macro"_cfg +
			c["padding"] + c["identifier"] + c["padding"] +
			"("_cfg + c["sfrag_argdef_list"] + ")"_cfg + c["padding"] +
			c["statement_e"] + c["padding_nl"];
		c.prim("sfrag_endmacro") =
			c["padding_b"] + c["statement_b"] +
			c["whitespace"] + "endmacro"_cfg + c["whitespace"] +
			c["statement_e"] + c["padding_nl"];
		c.prim("smacro") = c["sfrag_macro"] + c["lines"] + c["sfrag_endmacro"];

		c.prim("sinclude") =
			c["padding_b"] + c["statement_b"] +
			c["padding"] + "include"_cfg +
			c["padding"] + c["exp_str"] + c["padding"] +
			c["statement_e"] + c["padding_nl"];

		c.prim("statement") = c["sfor"] | c["sif"] | c["smacro"] | c["sinclude"];

		TRY(c.prep());

		return result;
	}

	util::Result<std::string, KError> TemplGen::codegen(
		std::string const &str,
		TemplObj const &obj
	) const {
		if (auto dict = obj.dict()) {
			return codegen(str, dict.value());
		} else {
			return dict.error();
		}
	}

	util::Result<std::string, KError> TemplGen::codegen(
		std::string const &str,
		TemplDict const &args
	) const {
		try {
			auto parser = SParser(_ctx);

			auto node = parser.parse(str, "file")->compressed().value();

			std::ofstream file("gen/templgen.gv");
			node.debug_dot(file);
			file.close();

			auto l_args = args;
			TRY(_add_builtin_identifiers(l_args));
			return _codegen(node, l_args, parser);
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
		TemplDict &args,
		SParser &parser
	) const {
		if (node.cfg_name() == "whitespace") {
			return _cg_default(node, args, parser);
		} else if (node.cfg_name() == "padding") {
			return _cg_default(node, args, parser);
		} else if (node.cfg_name() == "identifier") {
			return _cg_identifier(node, args, parser);
		} else if (node.cfg_name() == "padding_b") {
			return _cg_ref(node, args, parser);
		} else if (node.cfg_name() == "padding_e") {
			return _cg_ref(node, args, parser);
		} else if (node.cfg_name() == "padding_nl") {
			return _cg_ref(node, args, parser);
		} else if (node.cfg_name() == "raw") {
			return _cg_default(node, args, parser);
		} else if (node.cfg_name() == "line") {
			return _cg_line(node, args, parser);
		} else if (node.cfg_name() == "lines") {
			return _cg_lines(node, args, parser);
		} else if (node.cfg_name() == "file") {
			return _cg_ref(node, args, parser);
		} else if (node.cfg_name() == "comment") {
			return _cg_comment(node, args, parser);
		} else if (node.cfg_name() == "expression") {
			return _cg_expression(node, args, parser);
		} else if (node.cfg_name() == "statement") {
			return _cg_ref(node, args, parser);
		} else if (node.cfg_name() == "sif") {
			return _cg_sif(node, args, parser);
		} else if (node.cfg_name() == "sfor") {
			return _cg_sfor(node, args, parser);
		} else if (node.cfg_name() == "smacro") {
			_cg_smacro(node, args, parser);
			return {""};
		} else if (node.cfg_name() == "sinclude") {
			return _cg_sinclude(node, args, parser);
		} else {
			return KError::codegen("Unimplimented AstNode type: " + node.cfg_name());
		}
	}

	util::Result<std::string, KError> TemplGen::_cg_default(
		AstNode const &node,
		TemplDict &args,
		SParser &parser
	) const {
		CG_ASSERT(node.children().size() == 0, "Children count of padding must be 0");
		return node.consumed();
	}

	util::Result<std::string, KError> TemplGen::_cg_ref(
		AstNode const &node,
		TemplDict &args,
		SParser &parser
	) const {
		CG_ASSERT(node.children().size() == 1, "Children count of codegen_ref must be 1");
		return _codegen(node.children()[0], args, parser);
	}

	util::Result<std::string, KError> TemplGen::_cg_identifier(
		AstNode const &node,
		TemplDict &args,
		SParser &parser
	) const {
		return KError::codegen("Identifier does not have a codegen implimentation");
	}

	util::Result<std::string, KError> TemplGen::_cg_line(
		AstNode const &node,
		TemplDict &args,
		SParser &parser
	) const {
		auto result = std::string();
		for (auto &child : node.children()) {
			if (auto str = _codegen(child, args, parser)) {
				result += str.value();
			} else {
				return str;
			}
		}
		result += node.consumed();
		return result;
	}

	util::Result<std::string, KError> TemplGen::_cg_lines(
		AstNode const &node,
		TemplDict &args,
		SParser &parser
	) const {
		auto result = std::string();
		for (auto &child : node.children()) {
			if (auto str = _codegen(child, args, parser)) {
				result += str.value();
			} else {
				return str;
			}
		}
		return result;
	}

	util::Result<std::string, KError> TemplGen::_cg_comment(
		AstNode const &node,
		TemplDict &args,
		SParser &parser
	) const {
		return {""};
	}

	util::Result<std::string, KError> TemplGen::_cg_expression(
		AstNode const &node,
		TemplDict &args,
		SParser &parser
	) const {
		try {
			auto result = std::string();

			for (auto &child : node.children()) {
				auto name = child.cfg_name();
				if (name == "padding_b" || name == "padding_e") {
					continue;
				} else if (name == "expression_b") {
					if (_tag_keep_padding(child, true).value()) {
						if (auto padding = node.child_with_cfg("padding_b")) {
							result += _codegen(padding.value(), args, parser).value();
						}
					}
				} else if (name == "expression_e") {
					if (_tag_keep_padding(child, true).value()) {
						if (auto padding = node.child_with_cfg("padding_e")) {
							result += _codegen(padding.value(), args, parser).value();
						}
					}
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
		TemplDict &args,
		SParser &parser
	) const {
		return KError::codegen("statment not implimented");
	}

	util::Result<std::string, KError> TemplGen::_cg_sif(
		AstNode const &node,
		TemplDict &args,
		SParser &parser
	) const {
		try {
		auto result = std::string();
		CG_ASSERT(node.cfg_name() == "sif", "INTERNAL func can only parser sif nodes");

		for (auto &child : node.children()) {
			if (child.cfg_name() == "sif_start_chain") {
				auto if_node = child.child_with_cfg("sfrag_if").value();
				auto exp_node = if_node.child_with_cfg("exp").value();

				auto bool_value = _eval(exp_node, args).value();
				if (bool_value.boolean().value()) {
					//if statement could be empty
					if (auto lines_node = child.child_with_cfg("lines")) {
						auto scope = args;
						result += _codegen(lines_node.value(), scope, parser).value();
					}
					break;
				}
			} else if (child.cfg_name() == "sif_elif_chain") {
				auto elif_node = child.child_with_cfg("sfrag_elif").value();
				auto exp_node = elif_node.child_with_cfg("exp").value();

				auto bool_value = _eval(exp_node, args).value();
				if (bool_value.boolean().value()) {
					//elif statement could be empty
					if (auto lines_node = child.child_with_cfg("lines")) {
						auto scope = args;
						result += _codegen(lines_node.value(), scope, parser).value();
					}
					break;
				}
			} else if (child.cfg_name() == "sif_else_chain") {
				//else statement could be empty
				if (auto lines_node = child.child_with_cfg("lines")) {
					auto scope = args;
					result += _codegen(lines_node.value(), args, parser).value();
				}
				break;
			} else {
				continue;
			}
		}

		return result;
		} catch_kerror;
	}

	util::Result<std::string, KError> TemplGen::_cg_sfor(
		AstNode const &node,
		TemplDict &args,
		SParser &parser
	) const {
		try {
			auto result = std::string();


			auto sfrag_for = node.child_with_cfg("sfrag_for").value();
			auto iter_name = sfrag_for
				.child_with_cfg("exp_id")
				->child_with_cfg("identifier")
				->consumed();
			auto iter = sfrag_for.child_with_cfg("exp").value();
			auto lines = node.child_with_cfg("lines").value();

			auto iter_obj = _eval(iter, args)->list().value();
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
				result += _codegen(lines, local_args, parser).value();
				index++;
			}

			return result;
		} catch_kerror;
	}

	util::Result<void, KError> TemplGen::_cg_smacro(
		AstNode const &node,
		TemplDict &args,
		SParser &parser
	) const {
		try {
			auto arg_def = node.child_with_cfg("sfrag_macro").value();
			auto macro_name = arg_def.child_with_cfg("identifier")->consumed();
			auto macro_arg_list = arg_def.child_with_cfg("sfrag_argdef_list").value();
			auto content = node.child_with_cfg("lines").value();

			if (args.contains(macro_name)) {
				return KError::codegen(util::f(
					"Cannot create macro with name ",
					macro_name,
					" because identifier already exists"
				));
			}

			auto macro_args = std::vector<std::tuple<std::string, TemplObj>>();
			for (auto &macro_arg_node : macro_arg_list.children()) {
				auto macro_arg_name = macro_arg_node.child_with_cfg("identifier")->consumed();
				auto macro_arg_value = TemplObj();
				if (auto exp_node = macro_arg_node.child_with_cfg("exp")) {
					macro_arg_value = _eval(exp_node, args).value();
				}
				macro_args.push_back({macro_arg_name, macro_arg_value});
			}
			
			TemplFunc func = [this, macro_args, args, macro_name, content, &parser](TemplList l) -> TemplFuncRes {
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
				return {_codegen(content, local_args, parser).value()};
			};
			args[macro_name] = func;
			return {};
		} catch_kerror;
	}

	TemplGen::CodegenRes TemplGen::_cg_sinclude(
		AstNode const &node,
		TemplDict &args,
		SParser &parser
	) const {
		try {
			auto exp_str_node = node.child_with_cfg("exp_str");
			auto filename = _unpack_str(exp_str_node->consumed()).value();
			auto include_src = util::readEnvFile(filename);
			auto include_node = parser.parse(include_src, "file")->compressed().value();

			std::ofstream file("gen/templgen-include.gv");
			include_node.debug_dot(file);
			file.close();

			return _codegen(include_node, args, parser);
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval(
		util::Result<AstNode, KError> const &node,
		TemplDict const &args
	) const {
		if (node.has_value()) {
			return _eval(node.value(), args);
		} else {
			return node.error();
		}
	}

	TemplGen::EvalRes TemplGen::_eval(
		AstNode const &node,
		TemplDict const &args
	) const {
		auto name = node.cfg_name();
		if (name == "exp") {
			return _eval(node.children()[0], args);
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
			return KError::codegen("Unimplimented AstNode type: " + node.cfg_name());
		}
	}

	util::Result<TemplObj, KError> TemplGen::_eval_exp_sing(
		AstNode const &node,
		TemplDict const &args
	) const {
		CG_ASSERT(node.cfg_name() == "exp_sing", "");
		for (auto &child : node.children()) {
			auto name = child.cfg_name();
			if (name == "whitespace") {
				continue;
			} else if (name == "exp_id") {
				return _eval_exp_id(child, args);
			} else if (name == "exp_int") {
				return _eval_exp_int(child, args);
			} else if (name == "exp_str") {
				return _eval_exp_str(child, args);
			} else if (name == "exp") {
				return _eval(child, args);
			} else {
				return KError::codegen("Unknown node passed to _eval_exp_sing: " + child.cfg_name());
			}
		}
		return KError::codegen("exp_sing is an empty node");
	}
	util::Result<TemplObj, KError> TemplGen::_eval_exp_id(
		AstNode const &node,
		TemplDict const &args
	) const {
		try {
			auto name = node.child_with_cfg("identifier").value().consumed();
			if (args.count(name) == 0) {
				return KError::codegen("Unknown identifier \"" + name + "\"", node.location());
			}
			return args.at(name).dup().set_location(node.location());
		} catch_kerror;
	}
	util::Result<TemplObj, KError> TemplGen::_eval_exp_int(
		AstNode const &node,
		TemplDict const &args
	) const {
		try {
			CG_ASSERT(node.cfg_name() == "exp_int", "exp_ing must be passed to _eval_exp_int");
			auto value = TemplInt(0);
			for (auto c : node.consumed()) {
				value = value * 10 + c - '0';
			}
			auto value_obj = TemplObj(value).set_location(node.location());
			return {value_obj};
		} catch_kerror;
	}

	util::Result<TemplObj, KError> TemplGen::_eval_exp_str(
		AstNode const &node,
		TemplDict const &args
	) const {
		try {
			return TemplObj(_unpack_str(node.consumed()).value())
				.set_location(node.location());
		} catch_kerror;
	}

	util::Result<TemplObj, KError> TemplGen::_eval_exp1(
		AstNode const &node,
		TemplDict const &args
	) const {
		TemplDict l_args = args;
		try {
			auto exp = node.child_with_cfg("exp_sing").value();
			auto res = _eval(exp, l_args);
			CG_ASSERT(node.children().size() > 0, "Node must have at least one child");

			auto children = util::Adapt(node.children().begin() + 1, node.children().end());
			for (auto const &child : children) {
				if (child.cfg_name() == "exp_member") {
					l_args["self"] = res.value();
					res = _eval_exp_member(res.value(), child, l_args);
				} else if (child.cfg_name() == "exp_call") {
					res = _eval_exp_call(res.value(), child, l_args);
				} else {
					return KError::codegen("Unrecognized cfg node: " + child.cfg_name());
				}
			}
			return res;
		} catch_kerror;
	}

	util::Result<TemplObj, KError> TemplGen::_eval_exp_member(
		TemplObj const &lhs,
		AstNode const &node,
		TemplDict const &args
	) const {
		try {
			auto name = node.child_with_cfg("exp_id")->child_with_cfg("identifier")->consumed();
			return lhs.get_attribute(name);
		} catch_kerror;
	}

	util::Result<TemplObj, KError> TemplGen::_eval_exp_call(
		TemplObj const &lhs,
		AstNode const &node,
		TemplDict const &args
	) const {
		try {
			auto call_args = TemplList();
			if (args.contains("self")) {
				call_args.push_back(args.at("self"));
			}
			for (auto const &child : node.children()) {
				CG_ASSERT(child.cfg_name() == "exp", "Function call list must have exp nodes");
				call_args.push_back(_eval(child, args).value());
			}
			return lhs.func().value()(call_args);
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_exp2(
		AstNode const &node,
		TemplDict const &args
	) const {
		for (auto &child : node.children()) {
			auto name = child.cfg_name();
			auto exp2 = child.child_with_cfg("exp2");

			if (name == "whitespace") {
				continue;
			} else if (name == "exp_plus") {
				return +_eval(exp2, args);
			} else if (name == "exp_min") {
				return -_eval(exp2, args);
			} else if (name == "exp_log_not") {
				return !_eval(exp2, args);
			} else if (name == "exp1") {
				return _eval(child, args);
			} else {
				return KError::codegen("Unknown child in _eval_exp2: " + name);
			}
		}
		return KError::codegen("Empty node passed to _eval_exp2");
	}

	TemplGen::EvalRes TemplGen::_eval_exp3(
		AstNode const &node,
		TemplDict const &args
	) const {
		try {
			auto exp = node.child_with_cfg("exp2").value();
			auto res = _eval(exp, args);

			for (auto &child : node.children()) {
				auto name = child.cfg_name();
				auto exp2 = child.child_with_cfg("exp2");

				if (name == "whitespace") {
					continue;
				} else if (name == "exp2") {
					continue;
				} else if (name == "exp_mult") {
					res = res * _eval(exp2, args);
				} else if (name == "exp_div") {
					res = res / _eval(exp2, args);
				} else if (name == "exp_mod") {
					res = res % _eval(exp2, args);
				} else {
					return KError::codegen("Unknown child in _eval_exp3: " + name);
				}
			}
			return res;
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_exp4(
		AstNode const &node,
		TemplDict const &args
	) const {
		try {
			auto exp = node.child_with_cfg("exp3").value();
			auto res = _eval(exp, args);

			for (auto &child : node.children()) {
				auto name = child.cfg_name();
				auto exp3 = child.child_with_cfg("exp3");

				if (name == "whitespace") {
					continue;
				} else if (name == "exp3") {
					continue;
				} else if (name == "exp_add") {
					res = res + _eval(exp3, args);
				} else if (name == "exp_sub") {
					res = res - _eval(exp3, args);
				} else {
					return KError::codegen("Unknown child in _eval_exp4: " + name);
				}
			}
			return res;
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_exp6(
		AstNode const &node,
		TemplDict const &args
	) const {
		try {
			auto exp = node.child_with_cfg("exp4").value();
			auto res = _eval(exp, args);

			for (auto &child : node.children()) {
				auto name = child.cfg_name();
				auto exp4 = child.child_with_cfg("exp4");
				if (name == "whitespace") {
					continue;
				} else if (name == "exp4") {
					continue;
				} else if (name == "exp_comp_g") {
					res = res > _eval(exp4, args);
				} else if (name == "exp_comp_ge") {
					res = res >= _eval(exp4, args);
				} else if (name == "exp_comp_l") {
					res = res < _eval(exp4, args);
				} else if (name == "exp_comp_le") {
					res = res <= _eval(exp4, args);
				} else {
					return KError::codegen("Unknown child in _eval_exp6: " + name);
				}
			}
			return res;
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_exp7(
		AstNode const &node,
		TemplDict const &args
	) const {
		try {
			auto exp = node.child_with_cfg("exp6").value();
			auto res = _eval(exp, args);

			for (auto &child : node.children()) {
				auto name = child.cfg_name();
				auto exp6 = child.child_with_cfg("exp6");
				if (name == "whitespace") {
					continue;
				} else if (name == "exp6") {
					continue;
				} else if (name == "exp_comp_eq") {
					res = res == _eval(exp6, args);
				} else if (name == "exp_comp_neq") {
					res = res != _eval(exp6, args);
				} else {
					return KError::codegen("Unknown child in _eval_exp7: " + name);
				}
			}
			return res;
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_exp11(
		AstNode const &node,
		TemplDict const &args
	) const {
		try {
			auto exp = node.child_with_cfg("exp7").value();
			auto res = _eval(exp, args);

			for (auto &child : node.children()) {
				auto name = child.cfg_name();
				auto exp7 = child.child_with_cfg("exp7");
				if (name == "whitespace") {
					continue;
				} else if (name == "exp7") {
					continue;
				} else if (name == "exp_log_and") {
					res = res && _eval(exp7, args);
				} else {
					return KError::codegen("Unknown child in _eval_exp11: " + name);
				}
			}
			return res;
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_exp12(
		AstNode const &node,
		TemplDict const &args
	) const {
		try {
			auto exp = node.child_with_cfg("exp11").value();
			auto res = _eval(exp, args);

			for (auto &child : node.children()) {
				auto name = child.cfg_name();
				auto exp11 = child.child_with_cfg("exp11");

				if (name == "whitespace") {
					continue;
				} else if (name == "exp11") {
					continue;
				} else if (name == "exp_log_or") {
					res = res || _eval(exp11, args);
				} else {
					return KError::codegen("Unknown child in _eval_exp12: " + name);
				}
			}
			return res;
		} catch_kerror;
	}

	TemplGen::EvalRes TemplGen::_eval_filter_frag(
		TemplObj const &lhs,
		AstNode const &node,
		TemplDict const &args
	) const {
		try {
			CG_ASSERT(node.cfg_name() == "exp_filter_frag", "Must be an filter frag");
			auto filter = _eval_exp_id(node.child_with_cfg("exp_id").value(), args);
			auto l_args = args;
			l_args["self"] = lhs;
			if (auto call = node.child_with_cfg("exp_call")) {
				return _eval_exp_call(filter.value(), call.value(), l_args);
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
	) const {
		try {
			auto exp = node.child_with_cfg("exp12").value();
			auto res = _eval(exp, args);

			for (auto &child : node.children()) {
				auto name = child.cfg_name();
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
	) const {
		auto const &cons = node.consumed();
		auto const &name = node.cfg_name();
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
		} else if (name == "commend_e" || name == "expression_e") {
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
	) const {
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

	util::Result<void, KError> TemplGen::_add_builtin_identifiers(TemplDict &args) const {
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
