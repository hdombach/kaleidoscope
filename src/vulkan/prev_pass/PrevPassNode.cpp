#include <vector>

#include "PrevPass.hpp"
#include "PrevPassNode.hpp"
#include "PrevPass.hpp"
#include "vulkan/DescriptorSet.hpp"
#include "util/result.hpp"
#include "types/Node.hpp"
#include "types/ShaderResource.hpp"


namespace vulkan {
	PrevPassNode::VImpl PrevPassNode::VImpl::create_empty() {
		return {0, glm::vec3()};
	}

	util::Result<PrevPassNode, KError> PrevPassNode::create(
			Scene &scene,
			PrevPass &preview_pass,
			const Node *node)
	{
		auto result = PrevPassNode();

		result._id = node->id();
		result._node = node;
		result._prev_pass = &preview_pass;

		result._create_descriptor_sets();
		//TODO: This can be optimized by sharing some descriptors between nodes.

		return result;
	}

	void PrevPassNode::destroy() {
		_node = nullptr;
		_descriptor_set.destroy();
		_uniform.destroy();
	}

	void PrevPassNode::update() {
		_create_descriptor_sets();
	}

	PrevPassNode::VImpl PrevPassNode::vimpl() {
		auto mat = _node->get_matrix();
		return VImpl{
			_node->mesh().id(),
			_node->position(),
			glm::inverse(mat),
			mat,
		};
	}

	bool PrevPassNode::is_de() {
		return _node->mesh().has_de();
	}

	util::Result<void, KError> PrevPassNode::_create_descriptor_sets() {
		auto descriptor_templates = std::vector<DescriptorSetTemplate>();

		auto uniform = _node->resources().create_prim_uniform();
		TRY(uniform);
		_uniform = std::move(uniform.value());

		auto images = std::vector<VkImageView>();

		for (auto &resource : _node->resources().get()) {
			if (auto texture = resource->as_texture()) {
				images.push_back(texture.value().image_view());
			}
		}
		descriptor_templates.push_back(DescriptorSetTemplate::create_uniform(0, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, _uniform));
		if (auto temp = DescriptorSetTemplate::create_images(1, VK_SHADER_STAGE_FRAGMENT_BIT, images)) {
			descriptor_templates.push_back(temp.value());
		}

		auto descriptor_sets = DescriptorSets::create(
			descriptor_templates,
			1,
			_prev_pass->descriptor_pool());
		TRY(descriptor_sets);
		_descriptor_set = std::move(descriptor_sets.value());

		return {};
	}
}
