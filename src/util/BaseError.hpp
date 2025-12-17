#pragma once

#include <vector>
#include <string>
#include <ostream>

#include <vulkan/vulkan_core.h>

#include "util/FileLocation.hpp"
#include "util/IterAdapter.hpp"
#include "util/log.hpp"

namespace util {
	struct ErrorFrag {
		std::string type; std::string msg; util::FileLocation location;
	};
}

class BaseError {
	public:
		using SLoc = std::source_location;
		using FLoc = util::FileLocation;

		BaseError() = default;

		BaseError(
			std::string const &type,
			std::string const &msg,
			BaseError const &other,
			FLoc loc=SLoc::current()
		) {
			_frags = other._frags;
			_frags.push_back({type, msg, loc});
		}

		BaseError(
			std::string const &msg,
			BaseError const &other,
			FLoc loc=SLoc::current()
		) {
			_frags = other._frags;
			_frags.push_back({"", msg, loc});
		}

		BaseError(
			std::string const &type,
			std::string const &msg,
			FLoc loc=SLoc::current()
		) {
			_frags.push_back({type, msg, loc});
		}

		BaseError(
			std::string const &msg,
			FLoc loc=SLoc::current()
		) {
			_frags.push_back({"", msg, loc});
		}

		std::string const &msg() { return _frags.back().msg; }
		std::vector<util::ErrorFrag> const &frags() const { return _frags; }
		util::FileLocation loc() const { return _frags.back().location; }

		std::ostream &print(std::ostream &os) const {
			for (auto &f : util::reverse(_frags)) {
				os << "[";
				if (!f.type.empty()) {
					os << f.type << ":";
				}
				os << f.location << "] " << f.msg << std::endl;
			}
			return os;
		}

		std::string str() const {
			auto ss = std::stringstream();
			print(ss);
			return ss.str();
		};

	private:
		std::vector<util::ErrorFrag> _frags;
};

template<typename ErrorT>
	class TypedError: public BaseError {
		public:
			TypedError() = default;

			TypedError(
				ErrorT type,
				std::string const &msg,
				BaseError const &other,
				FLoc loc=SLoc::current()
			):
				BaseError(type_str(type), msg, other, loc),
				_type(type)
			{ }

			TypedError(
				ErrorT type,
				std::string const &msg,
				FLoc loc=SLoc::current()
			):
				BaseError(type_str(type), msg, loc),
				_type(type)
			{ }

			TypedError(
				ErrorT type,
				FLoc loc=SLoc::current()
			):
				BaseError(type_str(type), "", loc),
				_type(type)
			{ }

			static const char *type_str(ErrorT t);

			ErrorT type() const { return _type; }

		private:
			ErrorT _type;
	};

using VkError = TypedError<VkResult>;

template<>
	inline const char *VkError::type_str(VkResult t) {
		return "VkError todo";
	}

inline std::ostream &operator<<(std::ostream &os, VkResult vk_result) {
	return os << VkError::type_str(vk_result);
}

inline std::ostream &operator<<(std::ostream &os, BaseError const &e) {
	return e.print(os);
}

inline std::ostream &log_fatal_error(BaseError const &err) {
	return log_fatal_error(err.loc()) << err;
}

inline std::ostream &log_error(BaseError const &err) {
	return log_error(err.loc()) << err;
}
