#pragma once

#include <vector>
#include <string>
#include <ostream>

#include <vulkan/vulkan_core.h>

#include "util/FileLocation.hpp"
#include "util/IterAdapter.hpp"
#include "util/format.hpp"
#include "util/KError.hpp"

namespace util {
	struct ErrorFrag {
		std::string type; std::string msg; util::FileLocation location;
	};
}

template<typename ErrorT>
	class BaseError {
		public:
			using SLoc = std::source_location;
			using FLoc = util::FileLocation;

			BaseError() = default;

			template<typename T>
				BaseError(
					ErrorT type,
					std::string const &msg,
					BaseError<T> other,
					FLoc loc=SLoc::current()
				) {
					_frags = other.frags();
					_frags.push_back({type_str(type), msg, loc});
					_type = type;
				}

			BaseError(
				ErrorT type,
				std::string const &msg,
				KError const &error,
				FLoc loc=SLoc::current()
			) {
				_frags.push_back({"KError", error.what(), error.loc()});
				_frags.push_back({type_str(type), msg, loc});
			}

			BaseError(
				ErrorT type,
				std::string const &msg,
				FLoc loc=SLoc::current()
			) {
				_frags.push_back({type_str(type), msg, loc});
				_type = type;
			}

			static const char *type_str(ErrorT t);

			std::ostream &print(std::ostream &os) const {
				for (auto &f : util::reverse(_frags)) {
					os << "[" << f.type << ":" << f.location << "] " << f.msg << std::endl;
				}
				return os;
			}

			std::string msg() { return _frags.back().msg; }
			ErrorT type() const { return _type; }
			std::vector<util::ErrorFrag> const &frags() const { return _frags; }

		private:
			ErrorT _type;
			std::vector<util::ErrorFrag> _frags;
	};

using VkError = BaseError<VkResult>;

template<>
	inline const char *VkError::type_str(VkResult t) {
		return "VkError todo";
	}

inline std::ostream &operator<<(std::ostream &os, VkResult vk_result) {
	return os << VkError::type_str(vk_result);
}

template<typename T>
inline std::ostream &operator<<(std::ostream &os, BaseError<T> const &e) {
	return e.print(os);
}
