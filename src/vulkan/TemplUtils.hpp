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
}
