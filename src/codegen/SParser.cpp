#include "SParser.hpp"

#include "util/KError.hpp"
#include "util/StringRef.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"
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
		std::string const &root_node,
		std::string const &filename
	) {
		try {
			// _last_failure is a value specific to this function but it is easier to
			// pass it around everywhere as a property.
			// Should be fine since can't call multiple parses at same time.
			_last_failure = KError();
			auto ref = util::StringRef(str.c_str(), filename.c_str());
			//TODO: error handling for root
			auto node = _parse({ref, *_ctx.get(root_node)}).value();
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
		Stack s
	) {
		auto new_stack = Stack{s.str, s.node, &s};
		switch (s.node.type()) {
			case Type::none:
				return _set_failure(KError::codegen("Trying to parse none CfgNode", s.str.location()));
			case Type::literal:
				return _parse_lit(s);
			case Type::reference:
				return _parse_ref(s);
			case Type::sequence:
				return _parse_seq(s);
			case Type::alternative:
				return _parse_alt(s);
			case Type::closure:
				return _parse_cls(s);
			case Type::optional:
				return _parse_opt(s);
			case Type::negation:
				return _parse_neg(s);
		}
	}

	util::Result<AstNode, KError> SParser::_parse_lit(Stack s) {
		if (s.node.has_name()) {
			log_trace() << "Parsing lit: " << s.node.name() << std::endl;
		}

		size_t r = 0;
		auto res = AstNode(++_uid, _ctx, s.node.id(), s.str.location());
		for (auto c : s.node.content()) {
			if (s.str[r] != c) {
				if (s.str[r] == '\0') {
					auto msg = util::f(
						"Unexepcted EOF when parsing literal: ",
						util::escape_str(s.node.content())
					);
					return _set_failure(KError::codegen(msg, s.str.location()));
				} else {
					auto msg = util::f(
						"Unexpected character: ",
						s.str[r],
						" when parsing string: ",
						util::get_str_line(util::escape_str(s.str.str())),
						". Parse stack: "
					);
					auto i = &s;
					while (i) {
						if (i->node.has_name()) {
							msg += i->node.name() + " ";
						} else {
							msg += "<anon> ";
						}
						i = i->parent;
					}

					return _set_failure(KError::codegen(msg, s.str.location()));
				}
			}
			res.consume(c);
			r++;
		}
		return res;
	}

	util::Result<AstNode, KError> SParser::_parse_ref(Stack s) {
		if (s.node.has_name()) {
			log_trace() << "Parsing ref: " << s.node.name() << std::endl;
		}

		auto res = AstNode(++_uid, _ctx, s.node.id(), s.str.location());
		auto child_stack = Stack{s.str, _ctx.get(s.node.ref_id()), &s};
		if (auto child = _parse(child_stack)) {
			res.add_child(child.value());
		} else {
			return child.error();
		}
		return res;
	}

	util::Result<AstNode, KError> SParser::_parse_seq(Stack s) {
		if (s.node.has_name()) {
			log_trace() << "Parsing seq: " << s.node.name() << std::endl;
		}

		size_t r = 0;
		auto node = AstNode(++_uid, _ctx, s.node.id(), s.str.location());
		for (auto &child_cfg : s.node.children()) {
			auto child_stack = Stack{s.str + r, child_cfg, &s};
			if (auto child = _parse(child_stack)) {
				r += child.value().size();
				node.add_child(child.value());
			} else {
				return child.error();
			}
		}
		return node;
	}

	util::Result<AstNode, KError> SParser::_parse_alt(Stack s) {
		if (s.node.has_name()) {
			log_trace() << "Parsing alt: " << s.node.name() << std::endl;
		}

		auto node = AstNode(++_uid, _ctx, s.node.id(), s.str.location());
		for (auto &child_cfg : s.node.children()) {
			auto child_stack = Stack{s.str, child_cfg, &s};
			if (auto child = _parse(child_stack)) {
				node.add_child(child.value());
				return node;
			}
		}
		return _set_failure(_last_failure);
	}

	util::Result<AstNode, KError> SParser::_parse_cls(Stack s) {
		if (s.node.has_name()) {
			log_trace() << "Parsing cls: " << s.node.name() << std::endl;
		}

		auto node = AstNode(++_uid, _ctx, s.node.id(), s.str.location());
		size_t r = 0;
		while (true) {
			auto child_stack = Stack{s.str + r, s.node.children()[0], &s};
			if (auto child = _parse(child_stack)) {
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

	util::Result<AstNode, KError> SParser::_parse_opt(Stack s) {
		if (s.node.has_name()) {
			log_trace() << "Parsing opt: " << s.node.name() << std::endl;
		}

		auto node = AstNode(++_uid, _ctx, s.node.id(), s.str.location());
		if (auto child = _parse(Stack{s.str, s.node.children()[0], &s})) {
			node.add_child(child.value());
		}
		return node;
	}

	util::Result<AstNode, KError> SParser::_parse_neg(Stack s) {
		if (s.node.has_name()) {
			log_trace() << "Parsing neg: " << s.node.name() << std::endl;
		}

		auto node = AstNode(++_uid, _ctx, s.node.id(), s.str.location());
		if (s.str[0] == '\0') {
			return _set_failure(KError::codegen("Unexpected EOF", s.str.location()));
		}
		auto child_stack = Stack{s.str, s.node.children()[0], &s};
		if (auto c = _parse(child_stack)) {
			return _set_failure(KError::codegen(
				util::f("Unexpected element: ", s.str[0], " expected ", _ctx.node_str(s.node)),
				s.str.location()
			));
		} else {
			node.consume(s.str[0]);
		}
		return node;
	}
}
