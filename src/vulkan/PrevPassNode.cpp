#include <cstring>
#include <vector>

#include "../util/result.hpp"
#include "../types/Node.hpp"

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

		auto descriptor_templates = std::vector<DescriptorSetTemplate>();

		auto uniform = node->material().resources().create_prim_uniform();
		TRY(uniform);
		result._uniform = std::move(uniform.value());

		for (auto &resource : node->material().resources()) {
			if (!resource.is_primitive()) {
				descriptor_templates.push_back(DescriptorSetTemplate::create_image(1, VK_SHADER_STAGE_FRAGMENT_BIT, resource.image_view()));
			}
		}
		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, result._uniform));

		struct {
			bool operator()(DescriptorSetTemplate &lt, DescriptorSetTemplate &rt) const {
				return lt.layout_binding().binding < rt.layout_binding().binding;
			}
		} sort_templates;

		std::sort(descriptor_templates.begin(), descriptor_templates.end(), sort_templates);

		auto descriptor_sets = DescriptorSets::create(
			descriptor_templates,
			1,
			preview_pass.descriptor_pool());
		TRY(descriptor_sets);
		result._descriptor_set = std::move(descriptor_sets.value());

		//TODO: This can be optimized by sharing some descriptors between nodes.

		return result;
	}

	void PrevPassNode::destroy() {
		_node = nullptr;
		_descriptor_set.destroy();
		_uniform.destroy();
	}

	void PrevPassNode::update() {
		_node->material().resources().update_prim_uniform(
				_uniform,
				_node->resources().begin(),
				_node->resources().end());
	}
}
