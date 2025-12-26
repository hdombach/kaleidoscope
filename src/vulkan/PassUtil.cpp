#include "PassUtil.hpp"
#include "Error.hpp"

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

	util::Result<StaticBuffer, Error> create_material_buffer(Scene &scene) {
		auto material_range = max_material_range(scene.resource_manager().materials());
		auto buffer_range = material_range * scene.nodes().size();
		if (buffer_range == 0) {
			buffer_range = 1;
		}

		auto buf = std::vector<char>(buffer_range);
		size_t i = 0;
		for (auto &node : scene.nodes().raw()) {
			if (node) {
				auto &n = *node.get();
				n.resources().update_prim_uniform(buf.data() + i * material_range);
			}
			i++;
		}

		if (auto buffer = StaticBuffer::create(
				buf.data(), buffer_range,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
		)) {
			return buffer.move_value();
		} else {
			return Error(ErrorType::MISC, "Could not create material buffer", buffer.error());
		}
	}
}
