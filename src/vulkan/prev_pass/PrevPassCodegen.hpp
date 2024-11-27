#pragma once

#include <string>

#include "types/ShaderResource.hpp"

namespace vulkan::prev_pass {
	std::string cg_uniform_content(types::ShaderResources const &shader_resources);
}
