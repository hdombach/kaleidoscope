#pragma once

#include <iostream>
#include <string>
#include <vector>

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

			static CfgLeaf str(std::string const &str);
			static CfgLeaf include(std::string const &str);
			static CfgLeaf exclude(std::string const &str);
			static CfgLeaf var(std::string const &str);

			Type type() const { return _type; }

			/**
			 * @brief Matches against a string
			 * @returns number of characters consumed or 0 if there is mismatch
			 */
			uint32_t match(std::string const &str) const;

			std::string const &var_name() const { return _content; }

			std::ostream& print_debug(std::ostream &os) const;
			std::string str() const;
		private:
			CfgLeaf(Type type, std::string const &str, bool include);
		private:
			Type _type;
			std::string _content;
			bool _include;
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

			std::ostream& print_debug(std::ostream &os) const;
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
			std::vector<CfgRule> const &rules() const { return _rules; }

			std::ostream& print_debug(std::ostream &os) const;
		private:
			std::string _name;
			std::vector<CfgRule> _rules;
	};

	/**
	 * @brief A class for representing context free grammars
	 *
	 * CfgNodes are designed to be built by a CfgContext
	 *
	 * A CfgNode has three states
	 * - empty: The default initialization. Type is none and id is 0
	 * - anon: Used by temporary operations in the expressions. Will still have a 0 id
	 * - named: Has a name and id that is registered with CfgContext
	 *
	 * There is logic unique to each CfgNode, however, a significant amout of
	 * processing will be done outside the class so the benefits from method
	 * overriding are limited. Instead, CfgNodes are identiied by type.
	 */

	class CfgNode final {
		public:
			enum class Type {
				none,
				literal,
				reference,
				sequence,
				alternative,
				closure,
				optional,
				negation,
			};

			static const char *type_str(Type const &t);

		public:
			CfgNode();

			/*
			 * Because of how operators are used to construct cfg's, copy constructors
			 * are deleted to reduce unexpected behavior
			 */

			CfgNode(const CfgNode &other) = delete;
			CfgNode(CfgNode &&other);
			CfgNode& operator=(const CfgNode &other) = delete;
			CfgNode& operator=(CfgNode &&other);

			/**
			 * @brief Creates a string literal grammar object
			 * Matches against a string of the same value
			 * @param[in] str
			 */
			static CfgNode literal(std::string const &str);

			/**
			 * @brief Creates a reference by id 
			 * Is helpful when creating recursive definitions
			 * @param[in] other
			 */
			static CfgNode ref(uint32_t ref_id);

			/**
			 * @brief Creates a deferred reference by string
			 * Is helpful for when cfg isn't defined in ctx yet
			 */
			static CfgNode ref(std::string const &ref_name);

			/**
			 * @brief Creates a ref which refences the input node
			 */
			static CfgNode ref(CfgNode const &other);

			/**
			 * @brief Creates a sequence grammar object
			 * Matches against every child grammar object in order
			 * Will automatically combine lhs or rhs if able
			 */
			static CfgNode seq(CfgNode &&lhs, CfgNode &&rhs);

			/**
			 * @brief Creates an alternate grammar object
			 * Matches against just one of the child objects
			 * @param[in] lhs
			 * @param[in] rhs
			 */
			static CfgNode alt(CfgNode &&lhs, CfgNode &&rhs);

			/**
			 * @brief Creates a closure grammar object
			 * Matches against the child object multiple times or skips
			 * @param[in] c
			 */
			static CfgNode cls(CfgNode &&c);

			/**
			 * @brief Creates an optional grammar object
			 * Matches against the child object or skips
			 */
			static CfgNode opt(CfgNode &&c);

			/**
			 * @brief Creates a negation grammar object
			 * If child matches, will throw an error.
			 * If it doesn't, consume 1 char
			 */
			static CfgNode neg(CfgNode &&c);

			/**
			 * @brief Returns node to empty state
			 */
			void destroy();
			~CfgNode() { destroy(); }

			/**
			 * @brief is node in empty state
			 */
			bool has_value() const;

			operator bool() const { return has_value(); }

			CfgNode dup() const;

			/**
			 * @brief Whether the node has name and id registered with ctx
			 */
			bool has_name() const;

			/**
			 * @brief Adds name and id that are used by CfgContext
			 */
			void add_name(uint32_t id, std::string const &name);

			uint32_t id() const { return _id; }
			std::string const &name() const { return _name; }

			Type type() const { return _type; }
			std::vector<CfgNode> &children() { return _children; }
			std::vector<CfgNode> const &children() const { return _children; }
			std::string const &content() const { return _content; }

			void set_ref_id(uint32_t ref_id);
			uint32_t ref_id() const { return _ref_id; }
			std::string const &ref_name() const { return _ref_name; }

		private:
			uint32_t _id;
			std::string _name;
			Type _type;
			std::vector<CfgNode> _children;
			std::string _content;

			uint32_t _ref_id;
			std::string _ref_name;
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

	inline CfgNode operator ""_cfg(const char *str, size_t) {
		return CfgNode::literal(str);
	}

	inline std::ostream& operator<<(std::ostream& os, CfgNode::Type const &type) {
		os << CfgNode::type_str(type);
		return os;
	}
}
