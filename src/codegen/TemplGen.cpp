#include "TemplGen.hpp"
#include "codegen/SParser.hpp"
#include "codegen/TemplObj.hpp"
#include "util/KError.hpp"
#include "util/IterAdapter.hpp"

#include <fstream>

namespace cg {
	util::Result<TemplGen, KError> TemplGen::create() {
		auto result = TemplGen();
		auto &c = result._ctx;

		c["whitespace"] = c.cls(" "_cfg | "\t"_cfg | "\n"_cfg);
		c["padding"] = c.cls(" "_cfg | "\t"_cfg);
		c["digit"] =
			"0"_cfg | "1"_cfg | "2"_cfg | "3"_cfg | "4"_cfg |
			"5"_cfg | "6"_cfg | "7"_cfg | "8"_cfg | "9"_cfg;
		c["lower"] =
			"a"_cfg | "b"_cfg | "c"_cfg | "d"_cfg | "e"_cfg | "f"_cfg | "g"_cfg | "h"_cfg |
			"i"_cfg | "j"_cfg | "k"_cfg | "l"_cfg | "m"_cfg | "n"_cfg | "o"_cfg | "p"_cfg |
			"q"_cfg | "r"_cfg | "s"_cfg | "t"_cfg | "u"_cfg | "v"_cfg | "w"_cfg | "x"_cfg |
			"y"_cfg | "z"_cfg;
		c["upper"] =
			"A"_cfg | "B"_cfg | "C"_cfg | "D"_cfg | "E"_cfg | "F"_cfg | "G"_cfg | "H"_cfg |
			"I"_cfg | "J"_cfg | "K"_cfg | "L"_cfg | "M"_cfg | "N"_cfg | "O"_cfg | "P"_cfg |
			"Q"_cfg | "R"_cfg | "S"_cfg | "T"_cfg | "U"_cfg | "V"_cfg | "W"_cfg | "X"_cfg |
			"Y"_cfg | "Z"_cfg;
		c["alpha"] = c.ref("lower") | c.ref("uppoer");
		c["alnum"] = c.ref("alpha") | c.ref("digit");
		c["identifier"] = ("_"_cfg | c.ref("alpha")) + c.cls(c.ref("alnum") | "_"_cfg);

		c["padding_b"] = c.ref("padding");
		c["padding_e"] = c.ref("padding");
		c["padding_nl"] = c.ref("padding") + c.opt("\n"_cfg);

		c["raw"] = c.cls(!(
			c.ref("expression_b") |
			c.ref("statement_b") |
			c.ref("comment_b") |
			"\n"_cfg
		));
		c["line"] = c.cls(c.ref("statement") | c.ref("expression") | c.ref("comment") | c.ref("raw")) + c.opt("\n"_cfg);
		c["lines"] = c.cls(c.ref("line"));
		c["file"] = c.ref("lines");

		c["comment_b"] = "{#"_cfg + c.opt("-"_cfg | "+"_cfg);
		c["comment_e"] = c.opt("-"_cfg | "+"_cfg) + "#}"_cfg;
		c["comment"] =
			c.ref("padding_b") +
			c.ref("comment_b") + c.cls(!"#}"_cfg) + c.ref("comment_e") +
			c.ref("padding_e");

		c["expression_b"] = "{{"_cfg + c.opt("-"_cfg | "+"_cfg);
		c["expression_e"] = c.opt("-"_cfg | "+"_cfg) + "}}"_cfg;
		c["expression"] =
			c.ref("padding_b") + c.ref("expression_b") +
			c.ref("exp") +
			c.ref("expression_e") + c.ref("padding_e");
		c["exp"] = c.ref("exp_member");

		c["exp_id"] = c.ref("whitespace") + c.ref("identifier") + c.ref("whitespace");
		c["exp_member"] = c.ref("exp_id") + c.cls("."_cfg + c.ref("exp_id"));

		c["statement_b"] = "{%"_cfg + c.opt("-"_cfg | "+"_cfg);
		c["statement_e"] = c.opt("-"_cfg | "+"_cfg) + "%}"_cfg;

		c["sfrag_else"] =
			c.ref("padding_b") + c.ref("statement_b") +
			c.ref("whitespace") + "else"_cfg + c.ref("whitespace") +
			c.ref("statement_e") + c.ref("padding_nl");

		c["sfrag_if"] =
			c.ref("padding_b") + c.ref("statement_b") +
			c.ref("whitespace") + "if"_cfg + c.ref("whitespace") + c.ref("exp") + c.ref("whitespace") +
			c.ref("statement_e") + c.ref("padding_nl");
		c["sfrag_elif"] =
			c.ref("padding_b") + c.ref("statement_b") +
			c.ref("whitespace") + "elif"_cfg + c.ref("whitespace") + c.ref("exp") + c.ref("whitespace") +
			c.ref("statement_e") + c.ref("padding_nl");
		c["sfrag_endif"] =
			c.ref("padding_b") + c.ref("statement_b") +
			c.ref("whitespace") + "endif"_cfg + c.ref("whitespace") +
			c.ref("statement_e") + c.ref("padding_nl");
		c["sif_start_chain"] = c.ref("sfrag_if") + c.ref("lines");
		c["sif_elif_chain"] = c.ref("sfrag_elif") + c.ref("lines");
		c["sif_else_chain"] = c.ref("sfrag_else") + c.ref("lines");
		c["sif"] =
			c.ref("sif_start_chain") +
			c.cls(c.ref("sif_elif_chain")) +
			c.opt(c.ref("sif_else_chain")) +
			c.ref("sfrag_endif");

		c["sfrag_for"] =
			c.ref("padding_b") + c.ref("statement_b") +
			c.ref("padding") + "for"_cfg + c.ref("exp_id") + "in"_cfg + c.ref("exp") +
			c.ref("statement_e") + c.ref("padding_nl");
		c["sfrag_endfor"] =
			c.ref("padding_b") + c.ref("statement_b") +
			c.ref("whitespace") + "endfor"_cfg +  c.ref("whitespace") +
			c.ref("statement_e") + c.ref("padding_nl");
		c["sfor"] = c.ref("sfrag_for") + c.ref("lines") + c.ref("sfrag_endfor");

		c["statement"] = c.ref("sfor") | c.ref("sif");

		TRY(c.prep());

		result._prims = {
			"whitespace",
			"padding",
			"identifier",
			"padding_b",
			"padding_e",
			"padding_nl",
			"raw",
			"line",
			"lines",
			"file",
			"comment_b",
			"comment_e",
			"comment",
			"expression_b",
			"expression_e",
			"expression",
			"exp",
			"exp_id",
			"exp_member",
			"statement_b",
			"statement_e",
			"sfrag_if",
			"sfrag_elif",
			"sfrag_else",
			"sif_start_chain",
			"sif_elif_chain",
			"sif_else_chain",
			"sif",
			"sfrag_for",
			"sfor",
			"statement",
		};

		return result;
	}

	util::Result<std::string, KError> TemplGen::codegen(
		std::string const &str,
		TemplObj::Dict const &args
	) const {
		try {
			auto parser = SParser(_ctx);

			auto node = parser.parse(str, "file")->compressed(_prims).value();

			std::ofstream file("gen/templgen.gv");
			node.debug_dot(file);
			file.close();

			return _codegen(node, args);
		} catch_kerror;
	}

	#define CG_ASSERT(stm, msg) \
	if (!(stm)) {\
		return KError::codegen(msg); \
	}

	util::Result<std::string, KError> TemplGen::_codegen(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		if (node.cfg_name() == "whitespace") {
			return _cg_default(node, args);
		} else if (node.cfg_name() == "padding") {
			return _cg_default(node, args);
		} else if (node.cfg_name() == "identifier") {
			return _cg_identifier(node, args);
		} else if (node.cfg_name() == "padding_b") {
			return _cg_ref(node, args);
		} else if (node.cfg_name() == "padding_e") {
			return _cg_ref(node, args);
		} else if (node.cfg_name() == "padding_nl") {
			return _cg_ref(node, args);
		} else if (node.cfg_name() == "raw") {
			return _cg_default(node, args);
		} else if (node.cfg_name() == "line") {
			return _cg_line(node, args);
		} else if (node.cfg_name() == "lines") {
			return _cg_lines(node, args);
		} else if (node.cfg_name() == "file") {
			return _cg_ref(node, args);
		} else if (node.cfg_name() == "comment") {
			return _cg_comment(node, args);
		} else if (node.cfg_name() == "expression") {
			return _cg_expression(node, args);
		} else if (node.cfg_name() == "statement") {
			return _cg_ref(node, args);
		} else if (node.cfg_name() == "sif") {
			return _cg_sif(node, args);
		} else if (node.cfg_name() == "sfor") {
			return _cg_sfor(node, args);
		} else {
			return KError::codegen("Unimplimented AstNode type: " + node.cfg_name());
		}
	}

	util::Result<std::string, KError> TemplGen::_cg_default(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		CG_ASSERT(node.children().size() == 0, "Children count of padding must be 0");
		return node.consumed();
	}

	util::Result<std::string, KError> TemplGen::_cg_ref(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		CG_ASSERT(node.children().size() == 1, "Children count of codegen_ref must be 1");
		return _codegen(node.children()[0], args);
	}

	util::Result<std::string, KError> TemplGen::_cg_identifier(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		return KError::codegen("Identifier does not have a codegen implimentation");
	}

	util::Result<std::string, KError> TemplGen::_cg_line(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		auto result = std::string();
		for (auto &child : node.children()) {
			if (auto str = _codegen(child, args)) {
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
		TemplObj::Dict const &args
	) const {
		auto result = std::string();
		for (auto &child : node.children()) {
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
		TemplObj::Dict const &args
	) const {
		return {""};
	}

	util::Result<std::string, KError> TemplGen::_cg_expression(
		AstNode const &node,
		TemplObj::Dict const &args
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
							result += _codegen(padding.value(), args).value();
						}
					}
				} else if (name == "expression_e") {
					if (_tag_keep_padding(child, true).value()) {
						if (auto padding = node.child_with_cfg("padding_e")) {
							result += _codegen(padding.value(), args).value();
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
		TemplObj::Dict const &args
	) const {
		return KError::codegen("statment not implimented");
	}

	util::Result<std::string, KError> TemplGen::_cg_sif(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		try {
		auto result = std::string();
		CG_ASSERT(node.cfg_name() == "sif", "INTERNAL func can only parser sif nodes");

		for (auto &child : node.children()) {
			if (child.cfg_name() == "sif_start_chain") {
				auto if_node = child.child_with_cfg("sfrag_if").value();
				auto exp_node = if_node.child_with_cfg("exp").value();

				auto bool_value = _eval(exp_node, args).value();
				if (bool_value.boolean()) {
					//if statement could be empty
					if (auto lines_node = child.child_with_cfg("lines")) {
						result += _codegen(lines_node.value(), args).value();
					}
					break;
				}
			} else if (child.cfg_name() == "sif_elif_chain") {
				auto elif_node = child.child_with_cfg("sfrag_elif").value();
				auto exp_node = elif_node.child_with_cfg("exp").value();

				auto bool_value = _eval(exp_node, args).value();
				if (bool_value.boolean()) {
					//elif statement could be empty
					if (auto lines_node = child.child_with_cfg("lines")) {
						result += _codegen(lines_node.value(), args).value();
					}
					break;
				}
			} else if (child.cfg_name() == "sif_else_chain") {
				//else statement could be empty
				if (auto lines_node = child.child_with_cfg("lines")) {
					result += _codegen(lines_node.value(), args).value();
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
		TemplObj::Dict const &args
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

			auto iter_obj = _eval(iter, args).value();
			CG_ASSERT(iter_obj.type() == TemplObj::Type::List, "Must pass list object into for loop");
			for (auto &i : iter_obj.list()) {
				auto local_args = args;
				local_args[iter_name] = i;
				result += _codegen(lines, local_args).value();
			}

			return result;
		} catch_kerror;
	}

	util::Result<TemplObj, KError> TemplGen::_eval(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		if (node.cfg_name() == "exp") {
			return _eval(node.children()[0], args);
		} else if (node.cfg_name() == "exp_id") {
			return _eval_exp_id(node, args);
		} else if (node.cfg_name() == "exp_member") {
			return _eval_exp_member(node, args);
		} else {
			return KError::codegen("Unimplimented AstNode type: " + node.cfg_name());
		}
	}

	util::Result<TemplObj, KError> TemplGen::_eval_exp_id(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		try {
			auto name = node.child_with_cfg("identifier").value().consumed();
			if (args.count(name) == 0) {
				return KError::codegen("Unknown identifier \"" + name + "\"");
			}
			return args.at(name);
		} catch_kerror;
	}

	util::Result<TemplObj, KError> TemplGen::_eval_exp_member(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		try {
			auto exp = node.child_with_cfg("exp_id").value();
			auto res = _eval(exp, args);

			auto children = util::Adapt(node.children().begin() + 1, node.children().end());
			for (auto const &child : children) {
				CG_ASSERT(child.cfg_name() == "exp_id", "Invalid child of exp_member");
				auto name = child.child_with_cfg("identifier")->consumed();
				res = res->get_attribute(name);
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
			return KError::codegen(util::f("Invalid tag ending: ", cons[2]));
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
			return KError::codegen(util::f("Invalid tag beggining: ", cons[0]));
		} else {
			return KError::codegen(util::f("Cannot get tag padding for cfg node ", name));
		}
	}
}
