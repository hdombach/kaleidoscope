#include <vector>

#include "PrevPass.hpp"
#include "PrevPassNode.hpp"
#include "PrevPass.hpp"
#include "vulkan/DescriptorSet.hpp"
#include "util/result.hpp"
#include "util/log.hpp"
#include "types/Node.hpp"
#include "types/ShaderResource.hpp"


namespace vulkan {
	PrevPassNode::VImpl PrevPassNode::VImpl::create_empty() {
		return {0, 0,glm::vec3()};
	}

	util::Result<PrevPassNode, PrevPassNode::Error> PrevPassNode::create(
			Scene &scene,
			PrevPass &preview_pass,
			const Node *node)
	{
		auto result = PrevPassNode();

		result._id = node->id();
		result._node = node;
		result._prev_pass = &preview_pass;

		if (auto err = result._create_descriptor_sets().move_or()) {
			return Error(ErrorType::MISC, "Could not create prev pass node Descriptor Sets", err.value());
		}
		//TODO: This can be optimized by sharing some descriptors between nodes.

		return result;
	}

	void PrevPassNode::destroy() {
		_node = nullptr;
		_descriptor_set.destroy();
		_uniform.destroy();
	}

	void PrevPassNode::update() {
		require(_create_descriptor_sets());
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

	util::Result<void, PrevPassNode::Error> PrevPassNode::_create_descriptor_sets() {
		if (auto err = _node->resources().create_prim_uniform().move_or(_uniform)) {
			return Error(ErrorType::RESOURCE, "Could not create prev pass node uniform", err.value());
		}

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
		if (auto err = builder.add_uniform(_uniform).move_or()) {
			return Error(ErrorType::RESOURCE, "Could not add uniform", err.value());
		}
		if (images.size() > 0) {
			if (auto err = builder.add_image(images).move_or()) {
				return Error(ErrorType::RESOURCE, "Could not add image", err.value());
			}
		}

		_descriptor_set = DescriptorSets::create(builder, _prev_pass->descriptor_pool()).move_value();

		return {};
	}
}

template<>
const char *vulkan::PrevPassNode::Error::type_str(vulkan::PrevPassNode::ErrorType t) {
	switch (t) {
		case vulkan::PrevPassNode::ErrorType::VULKAN:
			return "PrevPassNode.VULKAN";
		case vulkan::PrevPassNode::ErrorType::MISC:
			return "PrevPassNode.MISC";
		case vulkan::PrevPassNode::ErrorType::RESOURCE:
			return "PrevPassNode.RESOURCE";
	}
}

std::ostream &operator<<(std::ostream &os, vulkan::PrevPassNode::ErrorType t) {
	return os << vulkan::PrevPassNode::Error::type_str(t);
}
