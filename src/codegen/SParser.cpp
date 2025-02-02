#include "SParser.hpp"

#include "util/KError.hpp"
#include "util/StringRef.hpp"
#include "util/Util.hpp"
#include "util/result.hpp"
#include "CfgContext.hpp"
#include "AstNode.hpp"

namespace cg {
	SParser::SParser(CfgContext const &ctx):
		_uid(0),
		_ctx(ctx)
	{}

	util::Result<size_t, KError> SParser::match(
		std::string const &str,
		std::string const &root_node
	) {
		try {
			auto ref = util::StringRef(str.c_str(), "codegen");
			return parse(str, root_node)->size();
		} catch_kerror;
	}

	util::Result<AstNode, KError> SParser::parse(
		std::string const &str,
		std::string const &root_node
	) {
		try {
			auto ref = util::StringRef(str.c_str(), "codegen");
			auto node = _parse(ref, _ctx.get(root_node)).value();
			if (node.size() < str.size()) {
				return KError::codegen("Entire string was not matched");
			} else {
				return node;
			}
		} catch_kerror;
	}

	/***********************************
	 * Parser helper functions
	 * *********************************/

	util::Result<AstNode, KError> SParser::_parse(
		util::StringRef str,
		CfgNode const &node
	) {
		switch (node.type()) {
			case Type::none:
				return KError::codegen("Trying to parse none CfgNode", str.location());
			case Type::literal:
				return _parse_lit(str, node);
			case Type::reference:
				return _parse_ref(str, node);
			case Type::sequence:
				return _parse_seq(str, node);
			case Type::alternative:
				return _parse_alt(str, node);
			case Type::closure:
				return _parse_cls(str, node);
			case Type::optional:
				return _parse_opt(str, node);
			case Type::negation:
				return _parse_neg(str, node);
		}
	}

	util::Result<AstNode, KError> SParser::_parse_lit(
		util::StringRef str,
		CfgNode const &cfg
	) {
		size_t r = 0;
		auto res = AstNode(++_uid, _ctx, cfg.id(), str.location());
		for (auto c : cfg.content()) {
			if (str[r] != c) {
				if (str[r] == '\0') {
					auto msg = util::f(
						"Unexepcted EOF when parsing literal: ",
						util::escape_str(cfg.content())
					);
					return KError::codegen(msg, str.location());
				} else {
					return KError::codegen(util::f(
						"Unexpected character: ",
						str[r],
						" in string: \"",
						util::escape_str(str.str()),
						"\""
					), str.location());
				}
			}
			res.consume(c);
			r++;
		}
		return res;
	}

	util::Result<AstNode, KError> SParser::_parse_ref(
		util::StringRef str,
		CfgNode const &cfg
	) {
		auto res = AstNode(++_uid, _ctx, cfg.id(), str.location());
		if (auto child = _parse(str, _ctx.get(cfg.ref_id()))) {
			res.add_child(child.value());
		} else {
			return child.error();
		}
		return res;
	}

	util::Result<AstNode, KError> SParser::_parse_seq(
		util::StringRef str,
		CfgNode const &cfg
	) {
		size_t r = 0;
		auto node = AstNode(++_uid, _ctx, cfg.id(), str.location());
		for (auto &child_cfg : cfg.children()) {
			if (auto child = _parse(str+r, child_cfg)) {
				r += child.value().size();
				node.add_child(child.value());
			} else {
				return child.error();
			}
		}
		return node;
	}

	util::Result<AstNode, KError> SParser::_parse_alt(
		util::StringRef str,
		CfgNode const &cfg
	) {
		auto node = AstNode(++_uid, _ctx, cfg.id(), str.location());
		for (auto &child_cfg : cfg.children()) {
			if (auto child = _parse(str, child_cfg)) {
				node.add_child(child.value());
				return node;
			}
		}
		return KError::codegen("Unmatched alternative: " + _ctx.node_str(cfg), str.location());
	}

	util::Result<AstNode, KError> SParser::_parse_cls(
		util::StringRef str,
		CfgNode const &cfg
	) {
		auto node = AstNode(++_uid, _ctx, cfg.id(), str.location());
		size_t r = 0;
		while (true) {
			if (auto child = _parse(str + r, cfg.children()[0])) {
				auto s = child.value().size();
				if (s == 0) { // Can happen with nested closures
					return node; 
				}
				r += s;
				node.add_child(child.value());
			} else {
				return node;
			}
		}
	}

	util::Result<AstNode, KError> SParser::_parse_opt(
		util::StringRef str,
		CfgNode const &cfg
	) {
		auto node = AstNode(++_uid, _ctx, cfg.id(), str.location());
		if (auto child = _parse(str, cfg.children()[0])) {
			node.add_child(child.value());
		}
		return node;
	}

	util::Result<AstNode, KError> SParser::_parse_neg(
		util::StringRef str,
		CfgNode const &cfg
	) {
		auto node = AstNode(++_uid, _ctx, cfg.id(), str.location());
		if (str[0] == '\0') {
			return KError::codegen("Unexpected EOF", str.location());
		}
		if (auto c = _parse(str, cfg.children()[0])) {
			return KError::codegen("Unexpected element: " + _ctx.node_str(cfg), str.location());
		} else {
			node.consume(str[0]);
		}
		return node;
	}
}
