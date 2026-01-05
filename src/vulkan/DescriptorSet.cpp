#include "DescriptorSet.hpp"

#include <vulkan/vulkan_core.h>

namespace vulkan {
	using util::f;

	const char *descriptor_type_str(VkDescriptorType const &t) {
		switch (t) {
			case VK_DESCRIPTOR_TYPE_SAMPLER:
				return "VK_DESCRIPTOR_TYPE_SAMPLER";
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
			case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
				return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
			case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
				return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
				return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
				return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
			case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
				return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
			case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
				return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
				return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
			case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
				return "VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK";
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
				return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
			case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
				return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV";
			case VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM:
				return "VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM";
			case VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM:
				return "VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM";
			case VK_DESCRIPTOR_TYPE_MUTABLE_EXT:
				return "VK_DESCRIPTOR_TYPE_MUTABLE_EXT";
			case VK_DESCRIPTOR_TYPE_MAX_ENUM:
				return "VK_DESCRIPTOR_TYPE_MAX_ENUM";
		}
	}
	
	util::Result<DescriptorSetLayout, Error> DescriptorSetLayout::create(
		std::vector<DescAttachment> const &attachments
	) {
		auto layout = DescriptorSetLayout();
		layout._desc_attachments = attachments;

		uint32_t i = 0;
		auto bindings = std::vector<VkDescriptorSetLayoutBinding>();

		for (auto &attachment : attachments) {
			VkDescriptorSetLayoutBinding binding;
			if (auto err = attachment.descriptor_binding().move_or(binding)) {
				return Error(
					ErrorType::SHADER_RESOURCE,
					util::f("Could not resolve descriptor set layout for binding ", i),
					err.value()
				);
			}
			binding.binding = i;
			bindings.push_back(binding);
			i++;
		}

		auto layout_info = VkDescriptorSetLayoutCreateInfo{};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.pBindings = bindings.data();
		layout_info.bindingCount = static_cast<uint32_t>(bindings.size());

		auto r= vkCreateDescriptorSetLayout(
			Graphics::DEFAULT->device(),
			&layout_info,
			nullptr,
			&layout._layout
		);

		if (r != VK_SUCCESS) {
			return Error(ErrorType::INVALID_ARG, "Could not create desriptor set layout", VkError(r));
		}

		return layout;
	}

	DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout &&other) {
		_layout = util::move_ptr(other._layout);
		_desc_attachments = std::move(other._desc_attachments);
	}

	DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout &&other) {
		destroy();

		_layout = util::move_ptr(other._layout);
		_desc_attachments = std::move(other._desc_attachments);

		return *this;
	}

	void DescriptorSetLayout::destroy() {
		if (_layout) {
			vkDestroyDescriptorSetLayout(
				Graphics::DEFAULT->device(),
				_layout,
				nullptr
			);
			_layout = nullptr;
		}
	}

	DescriptorSetLayout::~DescriptorSetLayout() { destroy(); }

	bool DescriptorSetLayout::has_value() const { return _layout; }

	DescriptorSetLayout::operator bool() const { return has_value(); }

	VkDescriptorSetLayout const &DescriptorSetLayout::layout() const {
		return _layout;
	}

	std::vector<DescAttachment> const &DescriptorSetLayout::desc_attachments() const {
		return _desc_attachments;
	}

	util::Result<DescriptorSets, Error> DescriptorSets::create(
		std::vector<DescAttachment> &attachments,
		DescriptorSetLayout const &layout,
		DescriptorPool const &pool
	) {
		if (!layout.has_value()) {
			return Error(ErrorType::INVALID_ARG, "Layout must be initialized");
		}
		if (!pool.descriptor_pool()) {
			return Error(ErrorType::INVALID_ARG, "Descriptor pool must be initialized");
		}

		auto set = DescriptorSets();
		set._descriptor_pool = &pool;

		auto alloc_info = VkDescriptorSetAllocateInfo{};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = pool.descriptor_pool();
		alloc_info.descriptorSetCount = 1;
		alloc_info.pSetLayouts = &layout.layout();

		set._descriptor_sets.resize(1);
		auto res = vkAllocateDescriptorSets(
			Graphics::DEFAULT->device(),
			&alloc_info,
			set._descriptor_sets.data()
		);
		if (res != VK_SUCCESS) {
			return Error(
				ErrorType::VULKAN,
				"Cannot allocate descriptor sets",
				VkError(res)
			);
		}

		auto writes = std::vector<VkWriteDescriptorSet>();
		int i = 0;
		for (auto &attachment : attachments) {
			auto write = VkWriteDescriptorSet{};
			if (auto err = attachment.descriptor_write().move_or(write)) {
				return Error(
					ErrorType::SHADER_RESOURCE,
					util::f("Could not resolve descriptor write for binding ", i),
					err.value()
				);
			}
			write.dstBinding = i;
			write.dstSet = set._descriptor_sets[0];
			writes.push_back(write);
			i++;
		}
		vkUpdateDescriptorSets(
			Graphics::DEFAULT->device(),
			static_cast<uint32_t>(writes.size()),
			writes.data(),
			0,
			nullptr
		);

		return set;
	}

	DescriptorSets::DescriptorSets(DescriptorSets &&other) {
		_descriptor_sets = std::move(other._descriptor_sets);

		_descriptor_pool = other._descriptor_pool;
		other._descriptor_pool = nullptr;
	}

	DescriptorSets &DescriptorSets::operator=(DescriptorSets &&other) {
		destroy();

		_descriptor_sets = std::move(other._descriptor_sets);

		_descriptor_pool = other._descriptor_pool;
		other._descriptor_pool = nullptr;

		return *this;
	}

	void DescriptorSets::destroy() {
		if (_descriptor_sets.size() > 0) {
			vkFreeDescriptorSets(
				Graphics::DEFAULT->device(),
				_descriptor_pool->descriptor_pool(),
				_descriptor_sets.size(),
				_descriptor_sets.data()
			);
			_descriptor_sets.clear();
		}
	}

	DescriptorSets::~DescriptorSets() { destroy(); }

	bool DescriptorSets::has_value() const {
		return _descriptor_sets.size() > 0;
	}

	DescriptorSets::operator bool() const {
		return has_value();
	}

	VkDescriptorSet const DescriptorSets::descriptor_set(uint32_t frame_index) const {
		return _descriptor_sets[frame_index];
	}
}
