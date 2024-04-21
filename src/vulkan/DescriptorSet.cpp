#include "DescriptorSet.hpp"
#include <vulkan/vulkan_core.h>

namespace vulkan {
	DescriptorSetTemplate DescriptorSetTemplate::create_image(
			uint32_t binding,
			VkShaderStageFlags stage_flags,
			ImageView const &image_view)
	{
		auto result = DescriptorSetTemplate{};
		result._layout_binding.binding = binding;
		result._layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		result._layout_binding.descriptorCount = 1;
		result._layout_binding.stageFlags = stage_flags;
		result._layout_binding.pImmutableSamplers = nullptr;

		result._image_view = image_view.value();

		return result;
	}

	DescriptorSetTemplate DescriptorSetTemplate::_create_uniform_impl(
			uint32_t binding,
			VkShaderStageFlags stage_flags,
			std::vector<VkBuffer> &&buffers,
			size_t buffer_size)
	{
		auto result = DescriptorSetTemplate{};
		result._layout_binding.binding = binding;
		result._layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		result._layout_binding.descriptorCount = 1;
		result._layout_binding.stageFlags = stage_flags;
		result._layout_binding.pImmutableSamplers = nullptr;
		result._buffers = std::move(buffers);
		result._buffer_range = buffer_size;

		return result;

	}

	util::Result<DescriptorSets, KError> DescriptorSets::create(
			std::vector<DescriptorSetTemplate> &templates,
			uint32_t frame_count,
			DescriptorPool &descriptor_pool)
	{
		auto result = DescriptorSets();
		result._descriptor_pool = &descriptor_pool;

		auto layout_bindings = std::vector<VkDescriptorSetLayoutBinding>();
		for (auto templ : templates) {
			layout_bindings.push_back(templ.layout_binding());
		}

		auto layout_info = VkDescriptorSetLayoutCreateInfo{};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
		layout_info.pBindings = layout_bindings.data();

		auto res = vkCreateDescriptorSetLayout(
				Graphics::DEFAULT->device(), 
				&layout_info, 
				nullptr, 
				&result._descriptor_set_layout);

		if (res != VK_SUCCESS) {
			return {res};
		}

		auto layout_binding_vec = std::vector<VkDescriptorSetLayout>(
				frame_count,
				result._descriptor_set_layout);

		auto alloc_info = VkDescriptorSetAllocateInfo{};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = descriptor_pool.descriptor_pool();
		alloc_info.descriptorSetCount = static_cast<uint32_t>(frame_count);
		alloc_info.pSetLayouts = layout_binding_vec.data();

		result._descriptor_sets.resize(frame_count);
		res = vkAllocateDescriptorSets(
				Graphics::DEFAULT->device(),
				&alloc_info,
				result._descriptor_sets.data());
		if (res != VK_SUCCESS) {
			return {res};
		}

		for (size_t frame = 0; frame < frame_count; frame++) {
			auto descriptor_writes = std::vector<VkWriteDescriptorSet>();

			union WriteBufferInfo {
				VkDescriptorBufferInfo buffer_info;
				VkDescriptorImageInfo image_info;
			};
			/* literally just makes sure buffer's lifetime lasts outside for loop */
			auto write_buffer_infos = std::vector<WriteBufferInfo>();
			write_buffer_infos.resize(templates.size());

			size_t write_i = 0;
			for (auto templ : templates) {
				auto descriptor_write = VkWriteDescriptorSet{};
				descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptor_write.dstSet = result._descriptor_sets[frame];
				descriptor_write.dstBinding = templ.layout_binding().binding;
				descriptor_write.dstArrayElement = 0;
				descriptor_write.descriptorType = templ.layout_binding().descriptorType;
				descriptor_write.descriptorCount = 1;

				if (descriptor_write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
					auto &buffer_info = write_buffer_infos[write_i].buffer_info;
					buffer_info.buffer = templ.buffers()[frame];
					buffer_info.offset = 0;
					buffer_info.range = templ.buffer_range();
					descriptor_write.pBufferInfo = &buffer_info;
				} else if (descriptor_write.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
					auto image_info = write_buffer_infos[write_i].image_info;
					image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					image_info.imageView = templ.image_view();
					image_info.sampler = Graphics::DEFAULT->mainTextureSampler();
					descriptor_write.pImageInfo = &image_info;
				}
				descriptor_writes.push_back(descriptor_write);
				write_i++;
			}

			vkUpdateDescriptorSets(
					Graphics::DEFAULT->device(), 
					static_cast<uint32_t>(descriptor_writes.size()),
					descriptor_writes.data(),
					0,
					nullptr);
		}

		return result;
	}

	DescriptorSets::DescriptorSets(DescriptorSets &&other):
		_descriptor_pool(other._descriptor_pool)
	{
		_descriptor_sets = std::move(other._descriptor_sets);

		_descriptor_set_layout = other._descriptor_set_layout;
		other._descriptor_set_layout = nullptr;

		_descriptor_pool = other._descriptor_pool;
	}

	DescriptorSets& DescriptorSets::operator=(DescriptorSets &&other) {
		_descriptor_sets = std::move(other._descriptor_sets);

		_descriptor_set_layout = other._descriptor_set_layout;
		other._descriptor_set_layout = nullptr;

		_descriptor_pool = other._descriptor_pool;

		return *this;
	}

	DescriptorSets::DescriptorSets():
		_descriptor_pool(),
		_descriptor_sets(),
		_descriptor_set_layout(nullptr)
	{ }

	DescriptorSets::~DescriptorSets() {
		if (_descriptor_set_layout) {
			vkDestroyDescriptorSetLayout(
					Graphics::DEFAULT->device(), 
					_descriptor_set_layout, 
					nullptr);
			_descriptor_set_layout = nullptr;
		}

		if (_descriptor_sets.size() > 0) {
			vkFreeDescriptorSets(
					Graphics::DEFAULT->device(), 
					_descriptor_pool->descriptor_pool(), 
					_descriptor_sets.size(), 
					_descriptor_sets.data());
			_descriptor_sets.clear();
		}
	}

	VkDescriptorSet DescriptorSets::descriptor_set(uint32_t frame_index) {
		return _descriptor_sets[frame_index];
	}

	VkDescriptorSetLayout DescriptorSets::layout() {
		return _descriptor_set_layout;
	}

	VkDescriptorSetLayout *DescriptorSets::layout_ptr() {
		return &_descriptor_set_layout;
	}

	DescriptorPool &DescriptorSets::descriptor_pool() {
		return *_descriptor_pool;
	}
}
