#pragma once

#include <vector>
#include <string>
#include <ostream>

#include <vulkan/vulkan_core.h>

#include "util/FileLocation.hpp"
#include "util/IterAdapter.hpp"
#include "util/log.hpp"
#include "util/result.hpp"

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
		switch (t) {
			case VK_SUCCESS:
				return "VK_SUCCESS";
			case VK_NOT_READY:
				return "VK_NOT_READY";
			case VK_TIMEOUT:
				return "VK_TIMEOUT";
			case VK_EVENT_SET:
				return "VK_EVENT_SET";
			case VK_EVENT_RESET:
				return "VK_EVENT_RESET";
			case VK_INCOMPLETE:
				return "VK_INCOMPLETE";
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				return "VK_ERROR_OUT_OF_HOST_MEMORY";
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
			case VK_ERROR_INITIALIZATION_FAILED:
				return "VK_ERROR_INITIALIZATION_FAILED";
			case VK_ERROR_DEVICE_LOST:
				return "VK_ERROR_DEVICE_LOST";
			case VK_ERROR_MEMORY_MAP_FAILED:
				return "VK_ERROR_MEMORY_MAP_FAILED";
			case VK_ERROR_LAYER_NOT_PRESENT:
				return "VK_ERROR_LAYER_NOT_PRESENT";
			case VK_ERROR_EXTENSION_NOT_PRESENT:
				return "VK_ERROR_EXTENSION_NOT_PRESENT";
			case VK_ERROR_FEATURE_NOT_PRESENT:
				return "VK_ERROR_FEATURE_NOT_PRESENT";
			case VK_ERROR_INCOMPATIBLE_DRIVER:
				return "VK_ERROR_INCOMPATIBLE_DRIVER";
			case VK_ERROR_TOO_MANY_OBJECTS:
				return "VK_ERROR_TOO_MANY_OBJECTS";
			case VK_ERROR_FORMAT_NOT_SUPPORTED:
				return "VK_ERROR_FORMAT_NOT_SUPPORTED";
			case VK_ERROR_FRAGMENTED_POOL:
				return "VK_ERROR_FRAGMENTED_POOL";
			case VK_ERROR_UNKNOWN:
				return "VK_ERROR_UNKNOWN";
			case VK_ERROR_OUT_OF_POOL_MEMORY:
				return "VK_ERROR_OUT_OF_POOL_MEMORY";
			case VK_ERROR_INVALID_EXTERNAL_HANDLE:
				return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
			case VK_ERROR_FRAGMENTATION:
				return "VK_ERROR_FRAGMENTATION";
			case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
				return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
			case VK_PIPELINE_COMPILE_REQUIRED:
				return "VK_PIPELINE_COMPILE_REQUIRED";
			case VK_ERROR_SURFACE_LOST_KHR:
				return "VK_ERROR_SURFACE_LOST_KHR";
			case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
				return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
			case VK_SUBOPTIMAL_KHR:
				return "VK_SUBOPTIMAL_KHR";
			case VK_ERROR_OUT_OF_DATE_KHR:
				return "VK_ERROR_OUT_OF_DATE_KHR";
			case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
				return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
			case VK_ERROR_VALIDATION_FAILED_EXT:
				return "VK_ERROR_VALIDATION_FAILED_EXT";
			case VK_ERROR_INVALID_SHADER_NV:
				return "VK_ERROR_INVALID_SHADER_NV";
			case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
				return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
			case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
				return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
			case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
				return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
			case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
				return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
			case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
				return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
			case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
				return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
			case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
				return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
			case VK_ERROR_NOT_PERMITTED_KHR:
				return "VK_ERROR_NOT_PERMITTED_KHR";
			case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
				return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
			case VK_THREAD_IDLE_KHR:
				return "VK_THREAD_IDLE_KHR";
			case VK_THREAD_DONE_KHR:
				return "VK_THREAD_DONE_KHR";
			case VK_OPERATION_DEFERRED_KHR:
				return "VK_OPERATION_DEFERRED_KHR";
			case VK_OPERATION_NOT_DEFERRED_KHR:
				return "VK_OPERATION_NOT_DEFERRED_KHR";
			case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
				return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
			case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
				return "VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT";
			case VK_RESULT_MAX_ENUM:
				return "VK_RESULT_MAX_ENUM";
		}
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

namespace util {
	inline void require(VkResult result, std::string const &msg="", util::FileLocation const &loc=std::source_location()) {
		if (result != VK_SUCCESS) {
			auto &os = log_error(loc);
			if (msg.empty()) {
				os << msg;
			} else {
				os << "vulkan function failed: ";
			}
			os << VkError::type_str(result);
		}
	}
}
