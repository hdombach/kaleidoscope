#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "util/result.hpp"

namespace cg {
	/**
	 * @brief Represents a leaf node for the context free grammar
	 *
	 * There are three possible types
	 * - String literal
	 * - A set of characters to match against
	 * - var: a reference to another grammar definition
	 *
	 * A group of characters is techincally not an or opteration which doesn't
	 * follow the structure of a Context Free grammar. However, it should still
	 * fine since the parse depends on a character lookahead.
	 */
	class CfgLeaf final {
		public:
			enum class Type {
				none,
				str,
				set,
				var
			};

			CfgLeaf();

			/**
			 * @brief Creates a string constant leaf node
			 */
			static CfgLeaf str(std::string const &str);
			/**
			 * @brief Creates a character set matching against any character in str
			 */
			static CfgLeaf include(std::string const &str);
			/**
			 * @brief Creates a character set that excludes all characters in str
			 */
			static CfgLeaf exclude(std::string const &str);
			/**
			 * @brief Creates a reference to another rule set
			 */
			static CfgLeaf var(std::string const &str);

			Type type() const { return _type; }

			/**
			 * @brief Matches against a string
			 * @returns number of characters consumed or 0 if there is mismatch
			 */
			util::Result<uint32_t, void> match(std::string const &str) const;

			std::string const &str_content() const { return _content; }
			std::string const &var_name() const { return _content; }

			std::ostream& print_debug(std::ostream &os) const;
			std::string str() const;

			bool operator==(CfgLeaf const &other) const;
			bool operator!=(CfgLeaf const &other) const;
		private:
			CfgLeaf(Type type, std::string const &str, bool include);
		private:
			Type _type;
			std::string _content;
			bool _include = false;
	};

	/**
	 * @brief Descibes a single replacement rule for the context free grammar
	 */
	class CfgRule final {
		public:
			CfgRule() = default;
			CfgRule(CfgLeaf const &leaf);
			CfgRule(CfgRule const &lhs, CfgRule const &rhs);

			std::vector<CfgLeaf> const &leaves() const { return _leaves; }

			/**
			 * @brief Seperates leaves to not use string constants
			 */
			void seperate_leaves();

			std::ostream& print_debug(std::ostream &os) const;
			std::string str() const;
		private:
			std::vector<CfgLeaf> _leaves;
	};

	/**
	 * @brief A collection of alternal rules
	 */
	class CfgRuleSet final {
		public:
			CfgRuleSet() = default;

			CfgRuleSet(std::string const &name);

			CfgRuleSet& operator=(CfgRuleSet const &set);
			CfgRuleSet& operator=(CfgRule const &rule);
			CfgRuleSet& operator=(CfgLeaf const &leaf);

			void add_rule(CfgRule const &rule);
			void add_rules(CfgRuleSet const &set);

			void set_name(std::string const &name) { _name = name; }
			std::string const &name() const { return _name; }
			std::vector<CfgRule> &rules() { return _rules; }
			std::vector<CfgRule> const &rules() const { return _rules; }

			std::ostream& print_debug(std::ostream &os) const;
			std::string str() const;
		private:
			std::string _name;
			std::vector<CfgRule> _rules;
	};

	/******* CfgLeaf ********/

	inline std::ostream &operator<<(std::ostream &os, CfgLeaf const &leaf) {
		return leaf.print_debug(os);
	}

	/******** CfgRule *********/

	inline CfgRule operator + (CfgLeaf const &lhs, CfgLeaf const &rhs) {
		return CfgRule(lhs, rhs);
	}

	inline CfgRule operator + (CfgRule const &lhs, CfgLeaf const &rhs) {
		return CfgRule(lhs, rhs);
	}

	inline std::ostream &operator<<(std::ostream &os, CfgRule const &rule) {
		return rule.print_debug(os);
	}

	/*******8 Rule set ********/

	inline CfgRuleSet operator | (CfgRule const &lhs, CfgRule const &rhs) {
		auto set = CfgRuleSet();
		set.add_rule(lhs);
		set.add_rule(rhs);
		return set;
	}

	inline CfgRuleSet operator | (CfgRuleSet const &lhs, CfgRule const &rhs) {
		auto r = lhs;
		r.add_rule(rhs);
		return r;
	}

	inline std::ostream &operator<<(std::ostream &os, CfgRuleSet const &set) {
		return set.print_debug(os);
	}
}
