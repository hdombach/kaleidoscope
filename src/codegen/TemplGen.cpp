#include "TemplGen.hpp"
#include "codegen/SParser.hpp"
#include "codegen/TemplObj.hpp"
#include "util/KError.hpp"

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

		c["exp_id"] = c.ref("whitespace") + c.ref("identifier") + c.ref("whitespace");

		c["padding_b"] = c.ref("padding");
		c["padding_e"] = c.ref("padding");

		c["comment_b"] = "{#"_cfg + c.opt("-"_cfg | "+"_cfg);
		c["comment_e"] = c.opt("-"_cfg | "+"_cfg) + "#}"_cfg;
		c["comment"] =
			c.ref("padding_b") +
			c.ref("comment_b") + c.cls(!"#}"_cfg) + c.ref("comment_e") +
			c.ref("padding_e");

		c["expression_b"] = "{{"_cfg + c.opt("-"_cfg | "+"_cfg);
		c["expression_e"] = c.opt("-"_cfg | "+"_cfg) + "}}"_cfg;
		c["expression"] =
			c.ref("padding_b") +
			c.ref("expression_b") + c.ref("exp_id") + c.ref("expression_e") +
			c.ref("padding_e");

		c["raw"] = c.cls(!(c.ref("expression") | c.ref("comment") | "\n"_cfg));

		c["line"] = c.cls(c.ref("expression") | c.ref("comment") | c.ref("raw")) + "\n"_cfg;

		c["file"] = c.cls(c.ref("line"));

		TRY(c.prep());

		result._prims = {
			"whitespace",
			"padding",
			"padding_b",
			"padding_e",
			"identifier",
			"exp_id",
			"comment_b",
			"comment_e",
			"comment",
			"expression_b",
			"expression_e",
			"expression",
			"raw",
			"line"
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
			return _codegen_default(node, args);
		} else if (node.cfg_name() == "padding") {
			return _codegen_default(node, args);
		} else if (node.cfg_name() == "padding_b") {
			return _codegen_ref(node, args);
		} else if (node.cfg_name() == "padding_e") {
			return _codegen_ref(node, args);
		} else if (node.cfg_name() == "identifier") {
			return _codegen_identifier(node, args);
		} else if (node.cfg_name() == "exp_id") {
			return _codegen_exp_id(node, args);
		} else if (node.cfg_name() == "comment") {
			return _codegen_comment(node, args);
		} else if (node.cfg_name() == "expression") {
			return _codegen_expression(node, args);
		} else if (node.cfg_name() == "raw") {
			return _codegen_default(node, args);
		} else if (node.cfg_name() == "file") {
			return _codegen_file(node, args);
		} else if (node.cfg_name() == "line") {
			return _codegen_line(node, args);
		} else {
			return KError::codegen("Unimplimented AstNode type: " + node.cfg_name());
		}
	}

	util::Result<std::string, KError> TemplGen::_codegen_default(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		CG_ASSERT(node.children().size() == 0, "Children count of padding must be 0");
		return node.consumed();
	}

	util::Result<std::string, KError> TemplGen::_codegen_ref(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		CG_ASSERT(node.children().size() == 1, "Children count of codegen_ref must be 1");
		return _codegen(node.children()[0], args);
	}

	util::Result<std::string, KError> TemplGen::_codegen_identifier(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		return KError::codegen("Identifier does not have a codegen implimentation");
	}

	util::Result<std::string, KError> TemplGen::_codegen_exp_id(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		for (auto &child : node.children()) {
			auto name = child.cfg_name();
			if (name == "identifier") {
				return args.at(child.consumed()).str();
			}
		}
		return KError::codegen("No identifier");
	}

	util::Result<std::string, KError> TemplGen::_codegen_comment(
		AstNode const &node,
		TemplObj::Dict const &args
	) const {
		return {""};
	}

	util::Result<std::string, KError> TemplGen::_codegen_expression(
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
					result += _codegen(child, args).value();
				}
			}
			return result;
		} catch_kerror;
	}

	util::Result<std::string, KError> TemplGen::_codegen_file(
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

	util::Result<std::string, KError> TemplGen::_codegen_line(
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
