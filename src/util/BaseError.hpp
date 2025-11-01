#pragma once

#include <vector>
#include <string>
#include <ostream>

#include <vulkan/vulkan_core.h>

#include "util/FileLocation.hpp"
#include "util/IterAdapter.hpp"
#include "util/format.hpp"

class BaseError {
	public:
		using SLoc = std::source_location;
		using FLoc = util::FileLocation;


		std::ostream &print(std::ostream &os) const {
			for (auto &f : util::reverse(_frags)) {
				os << "[" << f.type << ":" << f.location << "] " << f.msg << std::endl;
			}
			return os;
		}
		std::string msg() { return _frags.back().msg; }

	protected:
		BaseError() = default;
		BaseError(
			std::string const &type,
			std::string const &msg,
			util::FileLocation location,
			std::optional<BaseError> const &other
		) {
			if (other) {
				_frags = other->_frags;
			}
			_frags.push_back({type, msg, location});
		}

	private:
		struct _Frag {
			std::string type; std::string msg; util::FileLocation location;
		};
		std::vector<_Frag> _frags;
};

inline std::ostream &operator<<(std::ostream &os, BaseError const &e) {
	return e.print(os);
}
