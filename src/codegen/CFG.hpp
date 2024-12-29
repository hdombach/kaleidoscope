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
				reference,
				sequence,
				alternative,
				closure,
			};

			using Container = std::vector<CFG>;

		public:
			CFG();

			CFG(const CFG &other) = delete;
			CFG(CFG &&other);
			CFG& operator=(const CFG &other) = delete;
			CFG & operator=(CFG&& other);

			void destroy();
			~CFG() { destroy(); }

			CFG dup() const;

			static CFG literal(std::string const &str);

			static CFG ref(CFG const &other);

			static CFG seq(CFG &&lhs, CFG &&rhs);

			static CFG alt(CFG &&lhs, CFG &&rhs);

			/**
			 * @brief create a closure around a single reference
			 * @param[in] c
			 */
			static CFG cls(CFG const &c);

			static CFG cls(CFG &&c);

			Container const &children() const;
			std::string const &content() const;
			Type type() const;
			void set_name(std::string const &str);

			std::ostream &debug(std::ostream &os) const;

			std::string str() const;
			operator std::string() const { return str(); }

		private:
			Container _children;
			CFG const *_ref = nullptr;
			std::string _content;
			Type _type;
			std::string _name;
	};

	inline CFG operator ""_cfg(const char * str, size_t) {
		return CFG::literal(str);
	}

	inline CFG operator+(CFG const &lhs, CFG const &rhs) {
		return CFG::seq(CFG::ref(lhs), CFG::ref(rhs));
	};
	inline CFG operator+(CFG &&lhs, CFG &&rhs) {
		return CFG::seq(std::move(lhs), std::move(rhs));
	}
	inline CFG operator+(CFG const &lhs, CFG &&rhs) {
		return CFG::seq(CFG::ref(lhs), std::move(rhs));
	}
	inline CFG operator+(CFG &&lhs, CFG const &rhs) {
		return CFG::seq(std::move(lhs), CFG::ref(rhs).dup());
	}

	inline CFG operator|(CFG const &lhs, CFG const &rhs) {
		return CFG::alt(CFG::ref(lhs), CFG::ref(rhs));
	}
	inline CFG operator|(CFG &&lhs, CFG &&rhs) {
		return CFG::alt(std::move(lhs), std::move(rhs));
	}
	inline CFG operator|(CFG const &lhs, CFG &&rhs) {
		return CFG::alt(CFG::ref(lhs), std::move(rhs));
	}
	inline CFG operator|(CFG &&lhs, CFG const &rhs) {
		return CFG::alt(std::move(lhs), CFG::ref(rhs));
	}
}

inline std::ostream &operator<<(std::ostream& os, cg::CFG const &cfg) {
	return cfg.debug(os);
}
