#include <vector>

#include "../util/result.hpp"

#include "PrevPass.hpp"
#include "PrevPassNode.hpp"
#include "DescriptorSet.hpp"
#include "PrevPass.hpp"

namespace vulkan {
	util::Result<PrevPassNode, KError> PrevPassNode::create(
			Scene &scene,
			PrevPass &preview_pass,
			const Node *node)
	{
		auto result = PrevPassNode();

		result._id = node->id();
		result._node = node;

		auto uniform = MappedUniform<glm::vec3>::create();
		TRY(uniform);
		result._uniform = std::move(uniform.value());
		result._uniform.value() = result._node->position();

		auto descriptor_templates = std::vector<DescriptorSetTemplate>();

		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(0, VK_SHADER_STAGE_VERTEX_BIT, result._uniform));

		auto descriptor_sets = DescriptorSets::create(descriptor_templates, 1, preview_pass.descriptor_pool());
		
		TRY(descriptor_sets);

		result._descriptor_set = std::move(descriptor_sets.value());

		return result;
	}

	void PrevPassNode::destroy() {
		_node = nullptr;
		_descriptor_set.clear();
		_uniform.destroy();
	}

	void PrevPassNode::update() {
		_uniform.set_value(_node->position());
	}
}
