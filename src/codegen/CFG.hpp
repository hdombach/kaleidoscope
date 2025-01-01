#pragma once

#include <vector>
#include <string>
#include <ostream>

namespace cg {
	/**
	 * @brief Class for building context free grammars
	 */
	class Cfg {
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

			using Container = std::vector<Cfg>;

		public:
			Cfg();

			Cfg(const Cfg &other) = delete;
			Cfg(Cfg &&other);
			Cfg& operator=(const Cfg &other) = delete;
			Cfg & operator=(Cfg&& other);

			void destroy();
			~Cfg() { destroy(); }

			Cfg dup() const;

			/**
			 * @brief Creates a string literal grammar object
			 * Matches against a string of the same value
			 * @param[in] str
			 */
			static Cfg literal(std::string const &str);

			/**
			 * @brief Creates a reference string grammar object
			 * Is helpful when creating recursive definitions
			 * @param[in] other
			 */
			static Cfg ref(Cfg const &other);

			/**
			 * @brief Creates a sequence grammar object
			 * Matches against every child grammar object in order
			 * If either the left or right side is a sequence, they will automatically
			 * be combined.
			 * If both left and right side a literals, they are combined
			 * @param[in] lhs
			 * @param[in] rhs
			 */
			static Cfg seq(Cfg &&lhs, Cfg &&rhs);

			/**
			 * @brief Creates an alternate grammar object
			 * Matches against just one of the child objects
			 * @param[in] lhs
			 * @param[in] rhs
			 */
			static Cfg alt(Cfg &&lhs, Cfg &&rhs);

			/**
			 * @brief Create a closure around a single reference
			 * @param[in] c
			 */
			static Cfg cls(Cfg const &c);

			/**
			 * @brief Creates a closure grammar object
			 * Matches against the child object multiple times or skips
			 * @param[in] c
			 */
			static Cfg cls(Cfg &&c);

			/**
			 * @brief Creates an optional around a single reference
			 * @param[in] c
			 */
			static Cfg opt(Cfg const &c);

			/**
			 * @brief Creates an optional around a grammar object
			 * Matches against child object or skips
			 * @param[in] c
			 */
			static Cfg opt(Cfg &&c);

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
			Cfg const &ref() const;

			std::ostream &debug(std::ostream &os) const;

			std::string str() const;
			operator std::string() const { return str(); }

		private:
			Container _children;
			Cfg const *_ref = nullptr;
			std::string _content;
			Type _type;
			std::string _name;
	};

	inline Cfg operator ""_cfg(const char * str, size_t) {
		return Cfg::literal(str);
	}

	inline Cfg operator+(Cfg const &lhs, Cfg const &rhs) {
		return Cfg::seq(Cfg::ref(lhs), Cfg::ref(rhs));
	};
	inline Cfg operator+(Cfg &&lhs, Cfg &&rhs) {
		return Cfg::seq(std::move(lhs), std::move(rhs));
	}
	inline Cfg operator+(Cfg const &lhs, Cfg &&rhs) {
		return Cfg::seq(Cfg::ref(lhs), std::move(rhs));
	}
	inline Cfg operator+(Cfg &&lhs, Cfg const &rhs) {
		return Cfg::seq(std::move(lhs), Cfg::ref(rhs).dup());
	}

	inline Cfg operator|(Cfg const &lhs, Cfg const &rhs) {
		return Cfg::alt(Cfg::ref(lhs), Cfg::ref(rhs));
	}
	inline Cfg operator|(Cfg &&lhs, Cfg &&rhs) {
		return Cfg::alt(std::move(lhs), std::move(rhs));
	}
	inline Cfg operator|(Cfg const &lhs, Cfg &&rhs) {
		return Cfg::alt(Cfg::ref(lhs), std::move(rhs));
	}
	inline Cfg operator|(Cfg &&lhs, Cfg const &rhs) {
		return Cfg::alt(std::move(lhs), Cfg::ref(rhs));
	}
	inline std::ostream &operator<<(std::ostream& os, cg::Cfg const &cfg) {
		return cfg.debug(os);
	}
}
