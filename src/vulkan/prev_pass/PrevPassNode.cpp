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
		return {0, 0,glm::vec3()};
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
			_node->material().id(),
			_node->position(),
			glm::inverse(mat),
			mat,
		};
	}

	bool PrevPassNode::is_de() {
		return _node->mesh().is_de();
	}

	util::Result<void, KError> PrevPassNode::_create_descriptor_sets() {
		auto uniform = _node->resources().create_prim_uniform();
		TRY(uniform);
		_uniform = std::move(uniform.value());

		auto images = std::vector<VkImageView>();

		for (auto &resource : _node->resources().get()) {
			if (auto texture = resource->as_texture()) {
				images.push_back(texture.value()->image_view());
			}
		}

		auto bindings = std::vector<VkDescriptorSetLayoutBinding>();
		bindings.push_back(descriptor_layout_uniform(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
		if (images.size() > 0) {
			bindings.push_back(descriptor_layout_images(VK_SHADER_STAGE_FRAGMENT_BIT, images.size()));
		}

		_descriptor_set_layout = DescriptorSetLayout::create(bindings).move_value();

		auto builder = _descriptor_set_layout.builder();
		TRY(builder.add_uniform(_uniform));
		if (images.size() > 0) {
			TRY(builder.add_image(images));
		}

		_descriptor_set = DescriptorSets::create(builder, _prev_pass->descriptor_pool()).move_value();

		return {};
	}
}
