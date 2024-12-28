#pragma once

#include <vector>
#include <string>
#include <ostream>

namespace cg {
	class CFG {
		public:
			enum class Type {
				none,
				literal,
				sequence,
				alternative,
			};

			using Container = std::vector<CFG>;

		public:
			CFG();

			CFG(std::string const &str) { *this = CFG::literal(str); }
			CFG(const char *str) { *this = CFG::literal(str); }
			static CFG literal(std::string const &str);

			static CFG seq(CFG const &lhs, CFG const &rhs);
			static CFG seq(CFG &&lhs, CFG &&rhs);

			static CFG alt(CFG const &lhs, CFG const &rhs);
			static CFG alt(CFG &&lhs, CFG &&rhs);

			Container const &children() const;
			std::string const &content() const;
			Type type() const;

			std::ostream &debug(std::ostream &os) const;

			std::string str() const;
			operator std::string() const { return str(); }

		private:
			Container _children;
			std::string _content;
			Type _type;
	};

	inline CFG operator+(CFG const &lhs, CFG const &rhs) {
		return CFG::seq(lhs, rhs);
	};
	inline CFG operator+(CFG &&lhs, CFG &&rhs) {
		return CFG::seq(lhs, rhs);
	}

	inline CFG operator|(CFG const &lhs, CFG const &rhs) {
		return CFG::alt(lhs, rhs);
	}
	inline CFG operator|(CFG &&lhs, CFG &&rhs) {
		return CFG::alt(lhs, rhs);
	}
}

inline std::ostream &operator<<(std::ostream& os, cg::CFG const &cfg) {
	return cfg.debug(os);
}
