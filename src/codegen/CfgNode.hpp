#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "util/result.hpp"
#include "Tokenizer.hpp"

namespace cg {
	class CfgRuleSet;
	class CfgRule;
	/**
	 * @brief Represents a leaf node for the context free grammar
	 *
	 * There are three possible types
	 * - Matches nothing
	 * - A reference to another grammar definition
	 * - A single token type to match against
	 *
	 */
	class CfgLeaf {
		public:
			using SetPtr = std::unique_ptr<CfgRuleSet>;

		public:
			enum class Type {
				empty,
				var,
				token,
				cls
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

			/**
			 * @brief Creates a closure containing a rule set
			 */
			static CfgLeaf cls(CfgRuleSet const &rule_set);
			static CfgLeaf cls(CfgRule const &rule);

			CfgLeaf(CfgLeaf const &other);
			CfgLeaf(CfgLeaf &&other);
			CfgLeaf &operator=(CfgLeaf const &other);
			CfgLeaf &operator=(CfgLeaf &&other);

			Type type() const;

			Token::Type token_type() const;
			/**
			 * @brief Get the name of the variable referenced
			 *
			 * Only valid if the token type is var
			 */
			std::string const &var_name() const;
			/**
			 * @brief Gets the rule set
			 *
			 * Only valid if the token type is a closure
			 */
			CfgRuleSet &rule_set();
			/**
			 * @brief Gets the rule set
			 *
			 * Only valid if the token type is a closure
			 */
			CfgRuleSet const &rule_set() const;

			/**
			 * @brief Prints a debug representation of the leaf to an ostream
			 */
			std::ostream& print_debug(std::ostream &os) const;
			/**
			 * @breif Gets a debug representation of the leaf
			 */
			std::string str() const;

			bool operator==(CfgLeaf const &other) const;
			bool operator!=(CfgLeaf const &other) const;
		private:
			Type _type;
			TType _token_type;
			std::string _var_name;
			std::unique_ptr<CfgRuleSet> _rule_set;
	};

	/**
	 * @brief Descibes a single replacement rule for the context free grammar
	 *
	 * Consists of a sequence of leaf nodes which must be matched against
	 */
	class CfgRule {
		public:
			/**
			 * @brief Creates an empty rule set
			 */
			CfgRule() = default;
			/**
			 * @brief Creates a rule set containing a single leaf node to match against
			 */
			CfgRule(CfgLeaf const &leaf);
			/**
			 * @brief Creates a CfgRule from a concatenation of two other rules
			 */
			CfgRule(CfgRule const &lhs, CfgRule const &rhs);
			/**
			 * @brief Creates a CfgRule directly from a list of leaves
			 */
			CfgRule(std::vector<CfgLeaf> const &leaves);
			/**
			 * @brief Creates a CfgRule directly from a list of tokens
			 */
			CfgRule(std::vector<Token::Type> const &tokens);

			std::vector<CfgLeaf> const &leaves() const;
			std::vector<CfgLeaf> &leaves();

			/**
			 * @brief Gets the uid
			 */
			uint32_t set_id() const;
			/**
			 * @brief Sets the uid
			 *
			 * Should only be set by the CfgContext
			 */
			void set_set_id(uint32_t id);


			/**
			 * @brief Prints the debug representation of the rule to an ostream
			 */
			std::ostream& print_debug(std::ostream &os) const;
			/**
			 * @brief Gets the debug representation of the rule
			 */
			std::string str() const;
			bool operator==(CfgRule const &other) const;
			bool operator!=(CfgRule const &other) const;
		private:
			std::vector<CfgLeaf> _leaves;
			uint32_t _set_id = 0;
	};

	/**
	 * @brief A collection of alternative rules
	 *
	 * Consists of a sequence of CfgRules. Only one of them must be matched.
	 * The rules are tested in the order they are added until the first one matches
	 */
	class CfgRuleSet {
		public:
			using RuleContainer = std::vector<CfgRule>;
			using iterator = RuleContainer::iterator;
			using const_iterator = RuleContainer::const_iterator;

			/**
			 * @brief Creates an empty rule set
			 */
			CfgRuleSet() = default;

			/**
			 * @brief Creates an empty rule set with a name
			 *
			 * The name can be referenced by a leaf var name
			 */
			CfgRuleSet(std::string const &name);
			/**
			 * @brief Creates a rule set with a name and list of rules
			 *
			 * The name can be referenced by a leaf var name
			 * The rule alternatives to match against
			 */
			CfgRuleSet(std::string const &name, std::vector<CfgRule> &&rules);

			/**
			 * @brief Sets the list of rules to the contents of another rule set
			 *
			 * It does not function like a normal assign operation in that the rule
			 * set name remains the same.
			 */
			CfgRuleSet& operator=(CfgRuleSet const &set);
			/**
			 * @brief Sets the list of rules to a single rule
			 *
			 * It does not function like a normal assign operation in that the rule
			 * set name remains the same.
			 */
			CfgRuleSet& operator=(CfgRule const &rule);
			/**
			 * @brief Sets the list of rules to a single rule constructed from a leaf
			 *
			 * It does not function like a normal assign operation in that the rule
			 * set name remains the same
			 */
			CfgRuleSet& operator=(CfgLeaf const &leaf);

			bool operator==(CfgRuleSet const &other) const;
			bool operator!=(CfgRuleSet const &other) const;

			/**
			 * @brief Appends a single rule to test against
			 */
			void add_rule(CfgRule const &rule);
			/**
			 * @brief Appends a list of rules to test against
			 */
			void add_rules(CfgRuleSet const &set);

			/**
			 * @brief Sets the rule set name
			 *
			 * The name can be reference by a leaf var name
			 */
			void set_name(std::string const &name) { _name = name; }
			std::string const &name() const { return _name; }
			std::vector<CfgRule> &rules() { return _rules; }
			std::vector<CfgRule> const &rules() const { return _rules; }

			/**
			 * @brief Updates the list of rules to be matched against
			 *
			 * The rules tested against in order
			 */
			void set_rules(std::vector<CfgRule> &&rules);

			iterator begin();
			const_iterator begin() const;
			iterator end();
			const_iterator end() const;

			/**
			 * @brief Prints a debug representation of the rule set to an ostream
			 */
			std::ostream& print_debug(std::ostream &os, bool multiline=false) const;
			/**
			 * @brief Gets a debug representaiton of the rule set
			 */
			std::string str(bool multiline=false) const;
		private:
			std::string _name;
			std::vector<CfgRule> _rules;
	};

	/**
	 * @brief A closure containing a CfgRuleSet
	 *
	 * Matches 0 or more instances of the CfgRuleSet
	 * Closures are removed by the time they are processed by the Parser
	 */
	class CfgClosure final {
		public:
			/**
			 * @brief Creates an empty closure
			 */
			CfgClosure() = default;
			/**
			 * @brief Creates a closure from a rule set
			 */
			CfgClosure(CfgRuleSet const &rule_set);

			CfgRuleSet const &rule_set() const;

		private:
			CfgRuleSet _rule_set;
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


	/******** Rule set ********/

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

