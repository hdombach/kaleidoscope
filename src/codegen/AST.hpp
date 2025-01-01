#pragma once

#include <ostream>
#include <vector>
#include <string>

#include "CFG.hpp"
#include "util/result.hpp"

/***************************
 * The AbSoluTe Solver
 ***************************/

namespace cg {
	struct ASTError {
		std::string str() const { return "ASTError"; }
	};
	inline std::ostream &operator << (std::ostream &os, ASTError const &error) {
		return os << error.str();
	}

	/**
	 * @brief Abstract syntax tree
	 * The type is specified by the cfg.
	 */
	class ASTNode {
		public:
			ASTNode();
			ASTNode(Cfg const &cfg);

			bool has_value() const;
			operator bool() const { return has_value(); }

			std::vector<ASTNode> const &children() const { return _children; }

			void add_child(ASTNode const &node);

			/**
			 * @brief The rule that was used to generate this node
			 */
			Cfg const &cfg() const { return *_cfg; }

			/**
			 * @brief Characters that were consumed by this node
			 */
			std::string const &consumed() const { return _consumed; }

			void consume(char c);

			/**
			 * @brief Number of characters by this node and any children node
			 */
			size_t size() const;

			bool is_ref() const;

			/**
			 * @brief Combines all nodes that aren't ref nodes
			 */
			void compress();

			std::ostream &debug(std::ostream &os) const;

			std::string str() const;

		private:
			Cfg const *_cfg;
			std::vector<ASTNode> _children;
			std::string _consumed;

			/**
			 * @biref The cached size
			 */
			mutable size_t _size;

		private:
			size_t _calc_size() const;

	};

	/**
	 * @brief Tests whether string matches context free grammar
	 * @returns Number of consumed characters
	 */
	inline util::Result<size_t, ASTError> match_cfg(std::string const &str, Cfg const &cfg) {
		return match_cfg(str.data(), cfg);
	}
	/**
	 * @brief Tests whether string matches context free grammar
	 * @returns Number of consumed characters
	 */
	util::Result<size_t, ASTError> match_cfg(const char *str, Cfg const &cfg);


	inline util::Result<ASTNode, ASTError> parse_cfg(std::string const &str, Cfg const &cfg) {
		return parse_cfg(str.data(), cfg);
	}
	util::Result<ASTNode, ASTError> parse_cfg(const char *str, Cfg const &cfg);

	inline std::ostream &operator<<(std::ostream &os, ASTNode const &node) {
		return node.debug(os);
	}
}
