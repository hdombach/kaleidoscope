#pragma once

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

	struct ASTNode {
		cfg *cfg;
		std::vector<ASTNode> children;
		std::string consumed;
	};

	/**
	 * @brief Tests whether string matches context free grammar
	 * @returns Number of consumed characters
	 */
	inline util::Result<size_t, ASTError> match_cfg(std::string const &str, cfg const &cfg) {
		return match_cfg(str.data(), cfg);
	}
	/**
	 * @brief Tests whether string matches context free grammar
	 * @returns Number of consumed characters
	 */
	util::Result<size_t, ASTError> match_cfg(const char *str, cfg const &cfg);
}
