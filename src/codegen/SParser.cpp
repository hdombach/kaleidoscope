#include "SParser.hpp"

#include "util/KError.hpp"
#include "util/StringRef.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"
#include "util/result.hpp"
#include "CfgContext.hpp"
#include "AstNode.hpp"

namespace cg {
	SParser::SParser(CfgContextTemp const &ctx):
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
			auto node = _parse(Stack(ref, "", nullptr), *_ctx.get(root_node)).value();
			if (node.size() < str.size()) {
				if (_last_failure.type() == KError::Type::UNKNOWN) {
					return KError::codegen("Not all characters were consumed");
				} else {
					return _last_failure;
				}
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
		Stack const &s,
		CfgRuleSet const &set
	) {
		log_trace() << "Parsing rule set: " << set.name() << std::endl;
		for (auto &rule : set.rules()) {
			if (auto node = _parse(s.child(0, set.name()), rule)) {
				return node;
			}
		}
		return _last_failure;
	}

	util::Result<AstNode, KError> SParser::_parse(
		Stack const &s,
		CfgRule const &rule
	) {
		log_trace() << "Parsing rule: " << rule << std::endl;
		try {
			auto node = AstNode::create_rule(++_uid, s.rule, s.str.location());
			uint32_t i = 0;
			for (auto &leaf : rule.leaves()) {
				node.add_child(_parse(s.child(node.size()), leaf).value());
			}
			return node;
		} catch_kerror;
	}

	util::Result<AstNode, KError> SParser::_parse(
		Stack const &s,
		CfgLeaf const &leaf
	) {
		if (leaf.type() == CfgLeaf::Type::var) {
			auto set = _ctx.get(leaf.var_name());
			if (set == nullptr) {
				auto msg = util::f(
					"Variable ",
					leaf.var_name(),
					" used in grammar but not defined"
				);
				return _set_failure(KError::codegen(msg));
			}
			return _parse(s.child(0, leaf.var_name()), *set);
		}

		auto parsed = leaf.match(s.str.str());
		if (parsed) {
			log_trace() << "leaf matched: " << leaf << std::endl;
			auto begin = s.str.str();
			auto end = s.str.str() + parsed.value();
			return AstNode::create_str(++_uid, {begin, end}, s.str.location());
		} else {
			log_trace() << "leaf " << leaf << " didn't match: " << "\"" << util::get_str_line(s.str.str()) << "\"" << std::endl;
			auto msg = util::f(
				"Expected ",
				leaf.str(),
				" but got ",
				"\"",
				util::get_str_line(util::escape_str(s.str.str())),
				"\""
			);
			return _set_failure(KError::codegen(msg));
		}
	}
}
