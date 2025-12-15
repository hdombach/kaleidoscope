#include "SParser.hpp"

#include "util/StringRef.hpp"
#include "util/log.hpp"
#include "util/log.hpp"
#include "util/result.hpp"
#include "CfgContext.hpp"
#include "AstNode.hpp"

namespace cg {
	SParser::Ptr SParser::create(std::unique_ptr<CfgContext> &&ctx) {
		auto parser = std::make_unique<SParser>();;
		parser->_ctx = std::move(ctx);
		return parser;
	}

	SParser::SParser(SParser &&other) {
		_ctx = std::move(other._ctx);
	}

	SParser &SParser::operator=(SParser &&other) {
		_ctx = std::move(other._ctx);
		return *this;
	}

	util::Result<AstNode*, Error> SParser::parse(
		util::StringRef const &str,
		ParserContext &parser_ctx
	) {
		return SParserInstance::parse(str, *_ctx, parser_ctx);
	}

	CfgContext const &SParser::cfg() const {
		return *_ctx;
	}

	CfgContext &SParser::cfg() {
		return *_ctx;
	}

	util::Result<AstNode*, Error> SParserInstance::parse(
		util::StringRef const &str,
		CfgContext const &cfg_ctx,
		ParserContext &parser_ctx
	) {
		auto instance = SParserInstance();
		instance._cfg_ctx = &cfg_ctx;
		instance._parser_ctx = &parser_ctx;

		auto &tokens = instance._parser_ctx->get_tokens(str);
		log_assert(static_cast<bool>(instance._cfg_ctx), "SParser is not initialized");
		// _last_failure is a value specific to this function but it is easier to
		// pass it around everywhere as a property.
		// Should be fine since can't call multiple parses at same time.
		instance._last_failure = Error(ErrorType::UNKNOWN);
		//TODO: error handling for root
		auto node = instance._parse(tokens, 0, *instance._cfg_ctx->get_root());
		if (!node.has_value()) return node;
		if (node.value()->leaf_count() < tokens.size()) {
			if (instance._last_failure.type() == ErrorType::UNKNOWN) {
				return Error(ErrorType::MISC, "Not all characters were consumed");
			} else {
				return instance._last_failure;
			}
		} else {
			return node;
		}
	}

	Error SParserInstance::_set_failure(Error const &failure) {
		if (_last_failure.type() == ErrorType::UNKNOWN) {
			_last_failure = failure;
		} else if (failure.loc() > _last_failure.loc()) {
			_last_failure = failure;
		}
		return failure;
	}

	/***********************************
	 * Parser helper functions
	 * *********************************/

	util::Result<AstNode*, Error> SParserInstance::_parse(
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

	util::Result<AstNode*, Error> SParserInstance::_parse(
		std::vector<Token> const &tokens,
		uint32_t i,
		CfgRule const &rule,
		std::string const &set_name
	) {
		log_trace() << "Parsing rule: " << rule << std::endl;

		auto &node = _parser_ctx->create_rule_node(set_name);
		for (auto &leaf : rule.leaves()) {
			if (auto child = _parse(tokens, i, leaf)) {
				i += child.value()->leaf_count();
				node.add_child(*child.value());
			} else {
				return _set_failure(child.error());
			}
		}
		return &node;
	}

	util::Result<AstNode*, Error> SParserInstance::_parse(
		std::vector<Token> const &tokens,
		uint32_t i,
		CfgLeaf const &leaf
	) {
		if (leaf.type() == CfgLeaf::Type::var) {
			auto set = _cfg_ctx->get(leaf.var_name());
			if (set == nullptr) {
				auto msg = util::f(
					"Variable ",
					leaf.var_name(),
					" used in grammar but not defined"
				);
				return _set_failure(Error(ErrorType::INVALID_GRAMMAR, msg));
			}
			return _parse(tokens, i, *set);
		} else if (leaf.type() == CfgLeaf::Type::empty) {
			return &_parser_ctx->create_node();
		}

		auto &token = tokens[i];

		if (leaf.token_type() == token.type()) {
			log_trace()
				<< "leaf " << leaf
				<< " matched: \"" << token << "\"" << std::endl;
			return &_parser_ctx->create_tok_node(token);
		} else {
			log_trace() << "leaf " << leaf << " didn't match: " << "\"" << token << "\"" << std::endl;
			auto msg = util::f(
				"Expected ",
				leaf.str(),
				" but got ",
				token
			);
			return _set_failure(Error(ErrorType::INVALID_GRAMMAR, msg));
		}
	}
}
