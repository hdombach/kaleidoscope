#pragma once

#include <vector>
#include <string>
#include <ostream>

#include "util/FileLocation.hpp"
#include "util/IterAdapter.hpp"
#include "util/format.hpp"

template<typename Type>
class Error {
	public:
		std::ostream &print(std::ostream &os) const {
			for (auto &f : util::reverse(_frags)) {
				os << "[" << f.type << ":" << f.location << "] " << f.msg << std::endl;
			}
		}
		std::string msg() { return _frags.back().msg; }
		Type type() { return _type; }

	protected:
		template<typename TOther>
		Error(Type type, std::string msg, util::FileLocation location, Error<TOther> other) {
			_frags = other._frags;
			_frags.push_back({util::f(type), msg, location});
			_type = type;
		}

		Error(Type type, std::string msg, util::FileLocation location) = default;

	private:
		Type _type;
		struct _Frag {
			std::string type; std::string msg; util::FileLocation location;
		};
		std::vector<_Frag> _frags;
};

template<typename T>
std::ostream operator<<(std::ostream &os, Error<T> const &e) {
	return e.print(os);
}
