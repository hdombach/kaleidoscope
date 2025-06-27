#include "SParser.hpp"

#include "util/KError.hpp"
#include "util/StringRef.hpp"
#include "util/Util.hpp"
#include "util/log.hpp"
#include "util/result.hpp"
#include "CfgContext.hpp"
#include "AstNode.hpp"

namespace cg {
	SParser::Ptr SParser::create(std::unique_ptr<CfgContext> &&ctx) {
		auto parser = std::make_unique<SParser>();;
		parser->_uid = 0;
		parser->_ctx = std::move(ctx);
		return parser;
	}

	SParser::SParser(SParser &&other) {
		_uid = other._uid;
		_ctx = std::move(other._ctx);
	}

	SParser &SParser::operator=(SParser &&other) {
		_uid = other._uid;
		_ctx = std::move(other._ctx);
		return *this;
	}

	util::Result<AstNode, KError> SParser::parse(
		std::vector<Token> const &tokens
	) {
		try {
			log_assert(static_cast<bool>(_ctx), "SParser is not initialized");
			// _last_failure is a value specific to this function but it is easier to
			// pass it around everywhere as a property.
			// Should be fine since can't call multiple parses at same time.
			_last_failure = KError();
			//TODO: error handling for root
			auto node = _parse(tokens, 0, *_ctx->get_root()).value();
			if (node.leaf_count() < tokens.size()) {
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

	CfgContext const &SParser::cfg() const {
		return *_ctx;
	}

	CfgContext &SParser::cfg() {
		return *_ctx;
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
		std::vector<Token> const &tokens,
		uint32_t i,
		CfgRuleSet const &set
	) {
		log_trace() << "Parsing rule set: " << set << std::endl;
		for (auto &rule : set.rules()) {
			if (auto node = _parse(tokens, i, rule, set.name())) {
				return node;
			}
		}
		log_trace() << "Rule set " << set.name() << " failed." << std::endl;
		return _last_failure;
	}

	util::Result<AstNode, KError> SParser::_parse(
		std::vector<Token> const &tokens,
		uint32_t i,
		CfgRule const &rule,
		std::string const &set_name
	) {
		log_trace() << "Parsing rule: " << rule << std::endl;

		auto node = AstNode::create_rule(++_uid, set_name);
		for (auto &leaf : rule.leaves()) {
			if (auto child = _parse(tokens, i, leaf)) {
				i += child->leaf_count();
				node.add_child(child.value());
			} else {
				return _set_failure(child.error());
			}
		}
		return node;
	}

	util::Result<AstNode, KError> SParser::_parse(
		std::vector<Token> const &tokens,
		uint32_t i,
		CfgLeaf const &leaf
	) {
		if (leaf.type() == CfgLeaf::Type::var) {
			auto set = _ctx->get(leaf.var_name());
			if (set == nullptr) {
				auto msg = util::f(
					"Variable ",
					leaf.var_name(),
					" used in grammar but not defined"
				);
				return _set_failure(KError::codegen(msg));
			}
			return _parse(tokens, i, *set);
		} else if (leaf.type() == CfgLeaf::Type::empty) {
			return AstNode();
		}

		auto &token = tokens[i];

		if (leaf.token_type() == token.type()) {
			log_trace()
				<< "leaf " << leaf
				<< " matched: \"" << token << "\"" << std::endl;
			return AstNode::create_str(++_uid, token.str_ref());
		} else {
			log_trace() << "leaf " << leaf << " didn't match: " << "\"" << token << "\"" << std::endl;
			auto msg = util::f(
				"Expected ",
				leaf.str(),
				" but got ",
				token
			);
			return _set_failure(KError::codegen(msg));
		}
	}
}
