#pragma once

#include "codegen/TemplObj.hpp"

namespace vulkan {
	inline cg::TemplObj templ_property(
		std::string const &type,
		std::string const &name
	) {
		return cg::TemplObj{
			{"uniform_type", type},
			{"name", name}
		};
	}

	inline cg::TemplObj templ_define(
		std::string const &name,
		std::string const &value
	) {
		return cg::TemplObj{
			{"name", name},
			{"value", value}
		};
	}
}
