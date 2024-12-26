#include <vector>

#include <vulkan/vulkan_core.h>

#include "DescriptorSet.hpp"

namespace vulkan {
	DescriptorSetTemplate DescriptorSetTemplate::create_image(
			uint32_t binding,
			VkShaderStageFlags stage_flags,
			VkImageView image_view)
	{
		return create_image(binding, stage_flags, image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	DescriptorSetTemplate DescriptorSetTemplate::create_image(
			uint32_t binding,
			VkShaderStageFlags stage_flags,
			VkImageView image_view,
			VkImageLayout image_layout)
	{
		auto result = DescriptorSetTemplate();

		result._layout_binding.binding = binding;
		result._layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		result._layout_binding.descriptorCount = 1;
		result._layout_binding.stageFlags = stage_flags;
		result._layout_binding.pImmutableSamplers = nullptr;

		auto image_info = VkDescriptorImageInfo{};
		image_info.imageLayout = image_layout;
		image_info.imageView = image_view;
		image_info.sampler = *Graphics::DEFAULT->main_texture_sampler();
		result._image_infos.push_back(image_info);

		result._descriptor_writes = VkWriteDescriptorSet{};
		result._descriptor_writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//dstSet not handeled here
		result._descriptor_writes.dstBinding = binding;
		result._descriptor_writes.dstArrayElement = 0;
		result._descriptor_writes.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		result._descriptor_writes.pImageInfo = result._image_infos.data();
		result._descriptor_writes.descriptorCount = result._image_infos.size();

		result._image_views = std::vector<VkImageView>{image_view};
		result._image_layout = image_layout;

		return result;
	}


	util::Result<DescriptorSetTemplate, KError> DescriptorSetTemplate::create_images(
			uint32_t binding,
			VkShaderStageFlags stage_flags,
			std::vector<VkImageView> const &image_views)
	{
		if (image_views.empty()) {
			return KError::invalid_arg("Descriptor set images is emty");
		}

		auto result = DescriptorSetTemplate{};
		result._layout_binding.binding = binding;
		result._layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		result._layout_binding.descriptorCount = image_views.size();
		result._layout_binding.stageFlags = stage_flags;
		result._layout_binding.pImmutableSamplers = nullptr;

		for (auto &image_view : image_views) {
			result._image_views.push_back(image_view);
		}
		result._image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		for (auto &image_view : image_views) {
			auto image_info = VkDescriptorImageInfo{};
			image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_info.imageView = image_view;
			image_info.sampler = *Graphics::DEFAULT->main_texture_sampler();
			result._image_infos.push_back(image_info);
		}

		result._descriptor_writes = VkWriteDescriptorSet{};
		result._descriptor_writes.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//dstSet not handeled here
		result._descriptor_writes.dstBinding = binding;
		result._descriptor_writes.dstArrayElement = 0;
		result._descriptor_writes.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		result._descriptor_writes.pImageInfo = result._image_infos.data();
		result._descriptor_writes.descriptorCount = result._image_infos.size();

		return std::move(result);
	}

	DescriptorSetTemplate DescriptorSetTemplate::create_image_target(
			uint32_t binding,
			VkShaderStageFlags stage_flags,
			VkImageView image_view)
	{
		auto result = DescriptorSetTemplate{};

		result._layout_binding.binding = binding;
		result._layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		result._layout_binding.descriptorCount = 1;
		result._layout_binding.stageFlags = stage_flags;
		result._layout_binding.pImmutableSamplers = nullptr;

		auto image_info = VkDescriptorImageInfo{};
		image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		image_info.imageView = image_view;
		image_info.sampler = *Graphics::DEFAULT->main_texture_sampler();
		result._image_infos.push_back(image_info);

		auto descriptor_write = VkWriteDescriptorSet{};
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		//dstSet not handeled here
		descriptor_write.dstBinding = binding;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptor_write.pImageInfo = result._image_infos.data();
		descriptor_write.descriptorCount = result._image_infos.size();
		result._descriptor_writes = descriptor_write;

		result._image_views = std::vector<VkImageView>{image_view};
		result._image_layout = VK_IMAGE_LAYOUT_GENERAL;

		return result;
	}

	util::Result<DescriptorSetTemplate, KError> DescriptorSetTemplate::create_storage_buffer(
			uint32_t binding,
			VkShaderStageFlags stage_flags,
			VkBuffer buffer,
			size_t range)
	{
		if (buffer == nullptr) {
			return KError::invalid_arg("Null buffer");
		}

		auto result = DescriptorSetTemplate{};
		result._layout_binding.binding = binding;
		result._layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		result._layout_binding.descriptorCount = 1;
		result._layout_binding.stageFlags = stage_flags;
		result._layout_binding.pImmutableSamplers = nullptr;

		auto buffer_info = VkDescriptorBufferInfo{};
		buffer_info.buffer = buffer;
		buffer_info.offset = 0;
		buffer_info.range = range;
		result._buffer_infos.push_back(buffer_info);
		
		auto descriptor_write = VkWriteDescriptorSet{};
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstBinding = binding;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_write.pBufferInfo = result._buffer_infos.data();
		descriptor_write.descriptorCount = result._buffer_infos.size();
		result._descriptor_writes = descriptor_write;

		result._buffers = std::vector<VkBuffer>{buffer};
		result._buffer_range = range;

		return result;
	}

	util::Result<DescriptorSetTemplate, KError> DescriptorSetTemplate::create_storage_buffer(
			uint32_t binding,
			VkShaderStageFlags stage_flags,
			StaticBuffer &static_buffer)
	{
		return create_storage_buffer(
				binding,
				stage_flags,
				static_buffer.buffer(),
				static_buffer.range());
	}

	DescriptorSetTemplate &DescriptorSetTemplate::set_sampler(Sampler const &sampler) {
		for (auto &image_info : _image_infos) {
			image_info.sampler = *sampler;
		}

		return *this;
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

		for (auto buffer : result._buffers) {
			auto buffer_info = VkDescriptorBufferInfo{};

			buffer_info.buffer = buffer;
			buffer_info.offset = 0;
			buffer_info.range = buffer_size;

			result._buffer_infos.push_back(buffer_info);

		}

		auto descriptor_write = VkWriteDescriptorSet{};
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstBinding = binding;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_write.pBufferInfo = result._buffer_infos.data();
		descriptor_write.descriptorCount = result._buffer_infos.size();
		result._descriptor_writes = descriptor_write;

		return result;

	}

	util::Result<DescriptorSetLayout, KError> DescriptorSetLayout::create(
			std::vector<DescriptorSetTemplate> &templates)
	{
		auto result = DescriptorSetLayout();

		auto layout_bindings = std::vector<VkDescriptorSetLayoutBinding>();
		for (auto &templ : templates) {
			layout_bindings.push_back(templ.layout_binding());
		}

		auto layout_info = VkDescriptorSetLayoutCreateInfo{};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
		layout_info.pBindings = layout_bindings.data();

		return create(layout_info);
	}

	util::Result<DescriptorSetLayout, KError> DescriptorSetLayout::create(
			VkDescriptorSetLayoutCreateInfo &layout_info)
	{
		auto result = DescriptorSetLayout();

		auto res = vkCreateDescriptorSetLayout(
				Graphics::DEFAULT->device(),
				&layout_info,
				nullptr,
				&result._descriptor_set_layout);

		if (res != VK_SUCCESS) {
			return {res};
		}

		return result;
	}

	DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout &&other) {
		_descriptor_set_layout = other._descriptor_set_layout;
		other._descriptor_set_layout = nullptr;
	}

	DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) {
		destroy();

		_descriptor_set_layout = other._descriptor_set_layout;
		other._descriptor_set_layout = nullptr;

		return *this;
	}

	DescriptorSetLayout::DescriptorSetLayout():
		_descriptor_set_layout(nullptr)
	{}

	void DescriptorSetLayout::destroy() {
		if (_descriptor_set_layout) {
			vkDestroyDescriptorSetLayout(
					Graphics::DEFAULT->device(), 
					_descriptor_set_layout, 
					nullptr);
			_descriptor_set_layout = nullptr;
		}
	}

	util::Result<DescriptorSets, KError> DescriptorSets::create(
			std::vector<DescriptorSetTemplate> &templates,
			uint32_t frame_count,
			DescriptorPool &descriptor_pool)
	{
		auto result = DescriptorSets();
		result._descriptor_pool = &descriptor_pool;

		if (auto layout = DescriptorSetLayout::create(templates)) {
			result._descriptor_set_layout = std::move(layout.value());
		} else {
			return layout.error();
		}

		auto layout_binding_vec = std::vector<VkDescriptorSetLayout>(
				frame_count,
				result._descriptor_set_layout.layout());

		auto alloc_info = VkDescriptorSetAllocateInfo{};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = descriptor_pool.descriptor_pool();
		alloc_info.descriptorSetCount = static_cast<uint32_t>(frame_count);
		alloc_info.pSetLayouts = layout_binding_vec.data();

		result._descriptor_sets.resize(frame_count);
		auto res = vkAllocateDescriptorSets(
				Graphics::DEFAULT->device(),
				&alloc_info,
				result._descriptor_sets.data());
		if (res != VK_SUCCESS) {
			return {res};
		}

		for (size_t frame = 0; frame < frame_count; frame++) {
			auto descriptor_writes = std::vector<VkWriteDescriptorSet>();

			size_t write_i = 0;
			for (auto &templ : templates) {
				auto descriptor_write = templ.descriptor_write();
				descriptor_write.dstSet = result._descriptor_sets[frame];
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
		_descriptor_pool(other._descriptor_pool),
		_descriptor_set_layout(std::move(other._descriptor_set_layout))
	{
		_descriptor_sets = std::move(other._descriptor_sets);

		_descriptor_pool = other._descriptor_pool;
	}

	DescriptorSets& DescriptorSets::operator=(DescriptorSets &&other) {
		destroy();

		_descriptor_sets = std::move(other._descriptor_sets);

		_descriptor_set_layout = std::move(other._descriptor_set_layout);

		_descriptor_pool = other._descriptor_pool;

		return *this;
	}

	DescriptorSets::DescriptorSets():
		_descriptor_pool(nullptr),
		_descriptor_sets(),
		_descriptor_set_layout()
	{ }

	DescriptorSets::~DescriptorSets() {
		destroy();
	}

	void DescriptorSets::destroy() {
		_descriptor_set_layout.destroy();

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
		return _descriptor_set_layout.layout();
	}

	VkDescriptorSetLayout *DescriptorSets::layout_ptr() {
		return _descriptor_set_layout.layout_ptr();
	}

	DescriptorPool &DescriptorSets::descriptor_pool() {
		return *_descriptor_pool;
	}

	bool DescriptorSets::is_cleared() {
		return _descriptor_pool == nullptr;
	}
}
