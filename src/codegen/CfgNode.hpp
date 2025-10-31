#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "util/result.hpp"
#include "Tokenizer.hpp"

namespace cg {
	/**
	 * @brief Represents a leaf node for the context free grammar
	 *
	 * There are three possible types
	 * - Matches nothing
	 * - A reference to another grammar definition
	 * - A single token type to match against
	 *
	 */
	class CfgLeaf final {
		public:
			enum class Type {
				empty,
				var,
				token,
			};

			using TType = Token::Type;
			
			/**
			 * @brief Creates a leaf node that doesn't match anything
			 */
			CfgLeaf();

			/**
			 * @brief Creates a leaf node that matches a single token
			 */
			CfgLeaf(TType type);

			/**
			 * @brief Creates a reference to another rule set
			 */
			static CfgLeaf var(std::string const &str);

			Type type() const { return _type; }

			Token::Type token_type() const { return _token_type; }
			std::string const &var_name() const { return _var_name; }

			std::ostream& print_debug(std::ostream &os) const;
			std::string str() const;

			bool operator==(CfgLeaf const &other) const;
			bool operator!=(CfgLeaf const &other) const;
		private:
			Type _type;
			TType _token_type;
			std::string _var_name;
	};

	/**
	 * @brief Descibes a single replacement rule for the context free grammar
	 */
	class CfgRule final {
		public:
			CfgRule() = default;
			CfgRule(CfgLeaf const &leaf);
			CfgRule(CfgRule const &lhs, CfgRule const &rhs);
			CfgRule(std::vector<CfgLeaf> const &leaves);
			CfgRule(std::vector<Token::Type> const &tokens);

			std::vector<CfgLeaf> const &leaves() const { return _leaves; }

			uint32_t set_id() const;
			void set_set_id(uint32_t id);


			std::ostream& print_debug(std::ostream &os) const;
			std::string str() const;
			bool operator==(CfgRule const &other) const;
			bool operator!=(CfgRule const &other) const;
		private:
			std::vector<CfgLeaf> _leaves;
			uint32_t _set_id;
	};

	/**
	 * @brief A collection of alternal rules
	 */
	class CfgRuleSet final {
		public:
			using RuleContainer = std::vector<CfgRule>;
			using iterator = RuleContainer::iterator;
			using const_iterator = RuleContainer::const_iterator;

			CfgRuleSet() = default;

			CfgRuleSet(std::string const &name);
			CfgRuleSet(std::string const &name, std::vector<CfgRule> &&rules);

			CfgRuleSet& operator=(CfgRuleSet const &set);
			CfgRuleSet& operator=(CfgRule const &rule);
			CfgRuleSet& operator=(CfgLeaf const &leaf);

			void add_rule(CfgRule const &rule);
			void add_rules(CfgRuleSet const &set);

			void set_name(std::string const &name) { _name = name; }
			std::string const &name() const { return _name; }
			std::vector<CfgRule> &rules() { return _rules; }
			std::vector<CfgRule> const &rules() const { return _rules; }

			void set_rules(std::vector<CfgRule> &&rules);

			iterator begin();
			const_iterator begin() const;
			iterator end();
			const_iterator end() const;

			std::ostream& print_debug(std::ostream &os, bool multiline=false) const;
			std::string str(bool multiline=false) const;
		private:
			std::string _name;
			std::vector<CfgRule> _rules;
	};

	/******** CfgRule *********/
	inline CfgRule operator + (Token::Type const &lhs, Token::Type const &rhs) {
		return CfgRule(CfgLeaf(lhs), CfgLeaf(rhs));
	}

	inline CfgRule operator + (CfgLeaf const &lhs, Token::Type const &rhs) {
		return CfgRule(lhs, CfgLeaf(rhs));
	}

	inline CfgRule operator + (CfgLeaf const &lhs, CfgLeaf const &rhs) {
		return CfgRule(lhs, rhs);
	}

	inline CfgRule operator + (CfgRule const &lhs, Token::Type const &rhs) {
		return CfgRule(lhs, CfgLeaf(rhs));
	}

	inline CfgRule operator + (CfgRule const &lhs, CfgLeaf const &rhs) {
		return CfgRule(lhs, rhs);
	}


	/*******8 Rule set ********/

	inline CfgRuleSet operator | (Token::Type const &lhs, Token::Type const &rhs) {
		auto set = CfgRuleSet();
		set.add_rule(CfgLeaf(lhs));
		set.add_rule(CfgLeaf(rhs));
		return set;
	}

	inline CfgRuleSet operator | (Token::Type const &lhs, CfgLeaf const &rhs) {
		auto set = CfgRuleSet();
		set.add_rule(CfgLeaf(lhs));
		set.add_rule(rhs);
		return set;
	}

	inline CfgRuleSet operator | (Token::Type const &lhs, CfgRule const &rhs) {
		auto set = CfgRuleSet();
		set.add_rule(CfgLeaf(lhs));
		set.add_rule(rhs);
		return set;
	}

	inline CfgRuleSet operator | (CfgLeaf const &lhs, Token::Type const &rhs) {
		auto set = CfgRuleSet();
		set.add_rule(lhs);
		set.add_rule(CfgLeaf(rhs));
		return set;
	}

	inline CfgRuleSet operator | (CfgLeaf const &lhs, CfgLeaf const &rhs) {
		auto set = CfgRuleSet();
		set.add_rule(lhs);
		set.add_rule(rhs);
		return set;
	}

	inline CfgRuleSet operator | (CfgLeaf const &lhs, CfgRule const &rhs) {
		auto set = CfgRuleSet();
		set.add_rule(lhs);
		set.add_rule(rhs);
		return set;
	}

	inline CfgRuleSet operator | (CfgRule const &lhs, CfgLeaf const &rhs) {
		auto set = CfgRuleSet();
		set.add_rule(lhs);
		set.add_rule(rhs);
		return set;
	}

	inline CfgRuleSet operator | (CfgRule const &lhs, CfgRule const &rhs) {
		auto set = CfgRuleSet();
		set.add_rule(lhs);
		set.add_rule(rhs);
		return set;
	}

	inline CfgRuleSet operator | (CfgRuleSet const &lhs, Token::Type const &rhs) {
		auto r = lhs;
		r.add_rule(CfgLeaf(rhs));
		return r;
	}

	inline CfgRuleSet operator | (CfgRuleSet const &lhs, CfgLeaf const &rhs) {
		auto r = lhs;
		r.add_rule(rhs);
		return r;
	}

	inline CfgRuleSet operator | (CfgRuleSet const &lhs, CfgRule const &rhs) {
		auto r = lhs;
		r.add_rule(rhs);
		return r;
	}

}

inline std::ostream &operator<<(std::ostream &os, cg::CfgLeaf const &leaf) {
	return leaf.print_debug(os);
}

inline std::ostream &operator<<(std::ostream &os, cg::CfgRule const &rule) {
	return rule.print_debug(os);
}

inline std::ostream &operator<<(std::ostream &os, cg::CfgRuleSet const &set) {
	return set.print_debug(os);
}

