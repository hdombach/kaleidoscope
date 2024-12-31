#pragma once

#include <vector>
#include <string>
#include <ostream>

namespace cg {
	/**
	 * @brief Class for building context free grammars
	 */
	class cfg {
		public:
			enum class Type {
				none,
				literal,
				reference,
				sequence,
				alternative,
				closure,
				optional,
			};

			using Container = std::vector<cfg>;

		public:
			cfg();

			cfg(const cfg &other) = delete;
			cfg(cfg &&other);
			cfg& operator=(const cfg &other) = delete;
			cfg & operator=(cfg&& other);

			void destroy();
			~cfg() { destroy(); }

			cfg dup() const;

			/**
			 * @brief Creates a string literal grammar object
			 * Matches against a string of the same value
			 * @param[in] str
			 */
			static cfg literal(std::string const &str);

			/**
			 * @brief Creates a reference string grammar object
			 * Is helpful when creating recursive definitions
			 * @param[in] other
			 */
			static cfg ref(cfg const &other);

			/**
			 * @brief Creates a sequence grammar object
			 * Matches against every child grammar object in order
			 * If either the left or right side is a sequence, they will automatically
			 * be combined.
			 * If both left and right side a literals, they are combined
			 * @param[in] lhs
			 * @param[in] rhs
			 */
			static cfg seq(cfg &&lhs, cfg &&rhs);

			/**
			 * @brief Creates an alternate grammar object
			 * Matches against just one of the child objects
			 * @param[in] lhs
			 * @param[in] rhs
			 */
			static cfg alt(cfg &&lhs, cfg &&rhs);

			/**
			 * @brief Create a closure around a single reference
			 * @param[in] c
			 */
			static cfg cls(cfg const &c);

			/**
			 * @brief Creates a closure grammar object
			 * Matches against the child object multiple times or skips
			 * @param[in] c
			 */
			static cfg cls(cfg &&c);

			/**
			 * @brief Creates an optional around a single reference
			 * @param[in] c
			 */
			static cfg opt(cfg const &c);

			/**
			 * @brief Creates an optional around a grammar object
			 * Matches against child object or skips
			 * @param[in] c
			 */
			static cfg opt(cfg &&c);

			/**
			 * @brief Gets children object excluding references
			 */
			Container const &children() const;
			/**
			 * @brief Gets the string a literal matches against
			 */
			std::string const &content() const;
			Type type() const;
			/**
			 * @brief Sets name used for debug printing
			 */
			void set_name(std::string const &str);
			cfg const &ref() const;

			std::ostream &debug(std::ostream &os) const;

			std::string str() const;
			operator std::string() const { return str(); }

		private:
			Container _children;
			cfg const *_ref = nullptr;
			std::string _content;
			Type _type;
			std::string _name;
	};

	inline cfg operator ""_cfg(const char * str, size_t) {
		return cfg::literal(str);
	}

	inline cfg operator+(cfg const &lhs, cfg const &rhs) {
		return cfg::seq(cfg::ref(lhs), cfg::ref(rhs));
	};
	inline cfg operator+(cfg &&lhs, cfg &&rhs) {
		return cfg::seq(std::move(lhs), std::move(rhs));
	}
	inline cfg operator+(cfg const &lhs, cfg &&rhs) {
		return cfg::seq(cfg::ref(lhs), std::move(rhs));
	}
	inline cfg operator+(cfg &&lhs, cfg const &rhs) {
		return cfg::seq(std::move(lhs), cfg::ref(rhs).dup());
	}

	inline cfg operator|(cfg const &lhs, cfg const &rhs) {
		return cfg::alt(cfg::ref(lhs), cfg::ref(rhs));
	}
	inline cfg operator|(cfg &&lhs, cfg &&rhs) {
		return cfg::alt(std::move(lhs), std::move(rhs));
	}
	inline cfg operator|(cfg const &lhs, cfg &&rhs) {
		return cfg::alt(cfg::ref(lhs), std::move(rhs));
	}
	inline cfg operator|(cfg &&lhs, cfg const &rhs) {
		return cfg::alt(std::move(lhs), cfg::ref(rhs));
	}
}

inline std::ostream &operator<<(std::ostream& os, cg::cfg const &cfg) {
	return cfg.debug(os);
}
