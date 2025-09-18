#include "PassUtil.hpp"

namespace vulkan {
	size_t max_material_range(const MaterialContainer &materials) {
		size_t res = 0;

		for (auto &material : materials) {
			size_t r = material->resources().range();
			if (r > res) {
				res = r;
			}
		}

		return res;
	}

	cg::TemplObj material_templobj(
		uint32_t id,
		MaterialContainer const &materials
	) {
		auto &material = materials[id];
		auto declarations = cg::TemplList();
		for (auto &resource : material->resources().get()) {
			declarations.push_back(resource->templ_declaration());
		}
		cg::TemplInt declaration_padding =
			(max_material_range(materials) - material->resources().range()) / 4;

		return cg::TemplObj{
			{"declarations", declarations},
			{"declaration_padding", declaration_padding},
			{"id", material->id()},
			{"frag_src", material->frag_shader_src()},
		};
	}
}
