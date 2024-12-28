#include "CFG.hpp"

#include "tests/Test.hpp"
#include <sstream>

namespace cg {
	CFG::CFG(): _type(Type::none) { }

	CFG CFG::literal(const std::string &str) {
		auto result = CFG();

		result._type = Type::literal;
		result._content = str;

		return result;
	}

	CFG CFG::seq(CFG const &lhs, CFG const &rhs) {
		auto tlhs = lhs;
		auto trhs = rhs;
		return seq(std::move(tlhs), std::move(trhs));
	}
	CFG CFG::seq(CFG &&lhs, CFG &&rhs) {
		auto result = CFG();

		if (lhs.type() == Type::literal && rhs.type() == Type::literal) {
			result._type = Type::literal;
			result._content = lhs.content() + rhs.content();

			return result;
		}

		if (lhs.type() == Type::none) {
			return rhs;
		}
		if (rhs.type() == Type::none) {
			return lhs;
		}

		result._type = Type::sequence;

		if (lhs.type() == Type::sequence) {
			result._children = std::move(lhs.children());
		} else {
			result._children.push_back(std::move(lhs));
		}

		if (rhs.type() == Type::sequence) {
			result._children.insert(
				result._children.end(),
				lhs.children().begin(),
				lhs.children().end()
			);
		} else {
			result._children.push_back(std::move(rhs));
		}

		return result;
	}

	CFG CFG::alt(CFG const &lhs, CFG const &rhs) {
		auto tlhs = lhs;
		auto trhs = rhs;
		return alt(std::move(tlhs), std::move(trhs));
	}
	CFG CFG::alt(CFG &&lhs, CFG &&rhs) {
		auto result = CFG();

		result._type = Type::alternative;

		if (lhs.type() == Type::none) {
			return rhs;
		}
		if (rhs.type() == Type::none) {
			return lhs;
		}

		if (lhs.type() == Type::alternative) {
			result._children = std::move(lhs.children());
		} else {
			result._children.push_back(std::move(lhs));
		}

		if (rhs.type() == Type::alternative) {
			result._children.insert(
				result._children.end(),
				rhs.children().begin(),
				rhs.children().end()
			);
		} else {
			result._children.push_back(std::move(rhs));
		}

		return result;
	}

	CFG::Container const &CFG::children() const {
		return _children;
	}
	std::string const &CFG::content() const {
		return _content;
	}
	CFG::Type CFG::type() const {
		return _type;
	}

	std::ostream &CFG::debug(std::ostream &os) const {
		bool first = true;
		switch (_type) {
			case Type::none:
				return os;
			case Type::literal:
				return os << "\"" << _content << "\"";
			case Type::sequence:
				for (auto &child : _children) {
					if (first) {
						first = false;
					} else {
						os << "+ ";
					}

					if (child.type() == Type::alternative) {
						os << "(";
					}
					os << child;
					if (child.type() == Type::alternative) {
						os << ")";
					}

					return os;
				}
			case Type::alternative:
				for (auto &child : _children) {
					if (first) {
						first = false;
					} else {
						os << " | ";
					}

					os << child;
				}
		}

		return os;
	}

	std::string CFG::str() const {
		std::stringstream ss;
		debug(ss);
		return ss.str();
	}

	TEST(cfg, literal) {
		auto literal_cfg = CFG("simpler cfg");
		EXPECT_EQ(literal_cfg.str(), "\"simpler cfg\"");

		CFG literal_cfg2 = "another simpler cfg";
		EXPECT_EQ(literal_cfg2.str(), "\"another simpler cfg\"");
	}

	TEST(cfg, concat_str) {
		auto first = CFG() + "f" + "ir" + "st";
		EXPECT_EQ(first.str(), "\"first\"");

		auto second = CFG() + "second" + CFG();
		EXPECT_EQ(second.str(), "\"second\"");

		auto firstsecond = first + second + CFG();
		EXPECT_EQ(firstsecond.str(), "\"firstsecond\"");
	}

	TEST(cfg, alt) {
		auto first = CFG("first");
		auto second = CFG("second");

		auto first_second = first | second;
		EXPECT_EQ(first_second.str(), "\"first\" | \"second\"");

		auto first_second_third = first | second | "third";
		EXPECT_EQ(first_second_third.str(), "\"first\" | \"second\" | \"third\"");

		auto first_second_third2 = first | (second | "third2");
		EXPECT_EQ(first_second_third2.str(), "\"first\" | \"second\" | \"third2\"");

		auto first_second_third3 = (first | second) | "third3";
		EXPECT_EQ(first_second_third3.str(), "\"first\" | \"second\" | \"third3\"");
	}

	TEST(cfg, alt_concat) {
		auto first = CFG("first");
		auto second = CFG("second");
		auto third = CFG("third");

		auto test1 = first + second | third;
	}
}


