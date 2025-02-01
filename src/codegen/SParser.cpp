#include "SParser.hpp"

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
		auto ref = util::StringRef(str.c_str(), "codegen");
		return _match(ref, _ctx.get(root_node));
	}

	util::Result<AstNode, KError> SParser::parse(
		std::string const &str,
		std::string const &root_node
	) {
		auto ref = util::StringRef(str.c_str(), "codegen");
		return _parse(ref, _ctx.get(root_node));
	}

	/************************************
	 * Match helper functions
	 ************************************/

	util::Result<size_t, KError> SParser::_match(
		util::StringRef str,
		CfgNode const &node
	) {
		switch (node.type()) {
			case Type::none:
				return 0;
			case Type::literal:
				return _match_lit(str, node);
			case Type::reference:
				return _match_ref(str, node);
			case Type::sequence:
				return _match_seq(str, node);
			case Type::alternative:
				return _match_alt(str, node);
			case Type::closure:
				return _match_cls(str, node);
			case Type::optional:
				return _match_opt(str, node);
			case Type::negation:
				return _match_neg(str, node);
		}
	}

	util::Result<size_t, KError> SParser::_match_lit(
		util::StringRef str,
		CfgNode const &node
	) {
		auto r = 0;
		for (auto c : node.content()) {
			if (str[r] != c) return KError::codegen("Cannot match literal", str.location());
			r++;
		}
		return r;
	}

	util::Result<size_t, KError> SParser::_match_ref(
		util::StringRef str,
		CfgNode const &node
	) {
		return _match(str, _ctx.get(node.ref_id()));
	}

	util::Result<size_t, KError> SParser::_match_seq(
		util::StringRef str,
		CfgNode const &node
	) {
		size_t r = 0;
		for (auto &child : node.children()) {
			if (auto i = _match(str+r, child)) {
				r += i.value();
			} else {
				return KError::codegen("Cannot match sequence", str.location());
			}
		}
		return r;
	}

	util::Result<size_t, KError> SParser::_match_alt(
		util::StringRef str,
		CfgNode const &node
	) {
		for (auto &child : node.children()) {
			if (auto i = _match(str, child)) {
				return i;
			} 
		}
		return KError::codegen("Cannot match alt", str.location());
	}

	util::Result<size_t, KError> SParser::_match_cls(
		util::StringRef str,
		CfgNode const &node
	) {
		size_t r = 0;
		while (true) {
			if (auto i = _match(str + r, node.children()[0])) {
				if (i.value() == 0) {
					return r;
				}
				r += i.value();
			} else {
				return r;
			}
		}
	}

	util::Result<size_t, KError> SParser::_match_opt(
		util::StringRef str,
		CfgNode const &node
	) {
		return _match(str, node.children()[0]).value(0);
	}

	util::Result<size_t, KError> SParser::_match_neg(
		util::StringRef str,
		CfgNode const &node
	) {
		if (str[0] == '\0') {
			return KError::codegen("Cannot match EOF", str.location());
		}
		if (auto res = _match(str, node.children()[0])) {
			return KError::codegen("Cannot match neg", str.location());
		} else {
			return 1;
		}
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
				return KError::codegen(util::f(
					"Unexpected character: ",
					c,
					" in string: \"",
					util::escape_str(str.str()),
					"\""
				), str.location());
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
			return KError::codegen("Unexpected EOF");
		}
		if (auto c = _parse(str, cfg.children()[0])) {
			return KError::codegen("Unexpected element: " + _ctx.node_str(cfg));
		} else {
			node.consume(str[0]);
		}
		return node;
	}
}
