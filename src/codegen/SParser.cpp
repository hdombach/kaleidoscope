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
			// _last_failure is a value specific to this function but it is easier to
			// pass it around everywhere as a property.
			// Should be fine since can't call multiple parses at same time.
			_last_failure = KError();
			auto ref = util::StringRef(str.c_str(), "codegen");
			//TODO: error handling for root
			auto node = _parse(ref, *_ctx.get(root_node)).value();
			if (node.size() < str.size()) {
				return _last_failure;
			} else {
				return node;
			}
		} catch_kerror;
	}

	KError SParser::_set_failure(KError const &failure) {
		if (_last_failure.type() == KError::Type::UNKNOWN) {
			_last_failure = failure;
		} else if (failure.loc() > _last_failure.loc()) {
			_last_failure = failure;
		}
		return failure;
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
				return _set_failure(KError::codegen("Trying to parse none CfgNode", str.location()));
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
					return _set_failure(KError::codegen(msg, str.location()));
				} else {
					return _set_failure(KError::codegen(util::f(
						"Unexpected character: ",
						str[r],
						" in string: \"",
						util::get_str_line(util::escape_str(str.str())),
						"\""
					), str.location()));
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
		return _set_failure(_last_failure);
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
			return _set_failure(KError::codegen("Unexpected EOF", str.location()));
		}
		if (auto c = _parse(str, cfg.children()[0])) {
			return _set_failure(KError::codegen("Unexpected element: " + _ctx.node_str(cfg), str.location()));
		} else {
			node.consume(str[0]);
		}
		return node;
	}
}
