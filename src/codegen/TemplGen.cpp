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

		c["comment"] =
			c.ref("padding") +
			"{#"_cfg + c.cls(!"#}"_cfg) + "#}"_cfg +
			c.ref("padding");

		c["expression"] =
			c.ref("padding") +
			"{{"_cfg + c.ref("exp_id") + "}}"_cfg +
			c.ref("padding");

		c["raw"] = c.cls(!(c.ref("expression") | c.ref("comment") | "\n"_cfg));

		c["line"] = c.cls(c.ref("expression") | c.ref("comment") | c.ref("raw")) + "\n"_cfg;

		c["file"] = c.cls(c.ref("line"));

		TRY(c.prep());

		result._prims = {
			"whitespace",
			"padding",
			"identifier",
			"exp_id",
			"comment",
			"expression",
			"raw"
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
		for (auto &child : node.children()) {
			auto name = child.cfg_name();
			if (name == "padding") {
				continue;
			} else {
				return _codegen(child, args);
			}
		}
		return KError::codegen("Empty expression");
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
				return result;
			}
		}
		return result;
	}
}
