#include "PrevPassCodegen.hpp"
#include "types/ShaderResource.hpp"

namespace vulkan::prev_pass {
	std::string cg_uniform_content(types::ShaderResources const &shader_resources) {
		auto result = std::string();
		for (auto const &resource : shader_resources.get()) {
			if (resource->is_primitive()) {
				result += "\t" + resource->declaration() + ";\n";
			} else if (resource->type() == types::ShaderResource::Type::Texture) {
				result += "\tuint " + resource->name() + ";\n";
			}
		}
		return result;
	}
}
