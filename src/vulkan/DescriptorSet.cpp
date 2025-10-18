#include "DescriptorSet.hpp"

#include <vulkan/vulkan_core.h>

namespace vulkan {
	VkDescriptorSetLayoutBinding descriptor_layout_uniform(
		VkShaderStageFlags stage_flags
	) {
		auto binding = VkDescriptorSetLayoutBinding{};
		binding.binding = DESCRIPTOR_BINDING_UNUSED;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		binding.descriptorCount = 1;
		binding.stageFlags = stage_flags;
		binding.pImmutableSamplers = nullptr;

		return binding;
	}

	VkDescriptorSetLayoutBinding descriptor_layout_image(
		VkShaderStageFlags stage_flags
	) {
		auto binding = VkDescriptorSetLayoutBinding{};
		binding.binding = DESCRIPTOR_BINDING_UNUSED;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.descriptorCount = 1;
		binding.stageFlags = stage_flags;
		binding.pImmutableSamplers = nullptr;
		return binding;
	}

	VkDescriptorSetLayoutBinding descriptor_layout_images(
		VkShaderStageFlags stage_flags,
		size_t image_count
	) {
		auto binding = VkDescriptorSetLayoutBinding{};
		binding.binding = DESCRIPTOR_BINDING_UNUSED;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.descriptorCount = image_count;
		binding.stageFlags = stage_flags;
		binding.pImmutableSamplers = nullptr;
		return binding;
	}

	VkDescriptorSetLayoutBinding descriptor_layout_image_target(
		VkShaderStageFlags stage_flags,
		size_t image_count
	) {
		auto binding = VkDescriptorSetLayoutBinding{};
		binding.binding = DESCRIPTOR_BINDING_UNUSED;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		binding.descriptorCount = 1;
		binding.stageFlags = stage_flags;
		binding.pImmutableSamplers = nullptr;
		return binding;
	}

	VkDescriptorSetLayoutBinding descriptor_layout_storage_buffer(
		VkShaderStageFlags stage_flags
	) {
		auto binding = VkDescriptorSetLayoutBinding{};
		binding.binding = DESCRIPTOR_BINDING_UNUSED;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		binding.descriptorCount = 1;
		binding.stageFlags = stage_flags;
		binding.pImmutableSamplers = nullptr;
		return binding;
	}
	
	util::Result<DescriptorSetLayout, KError> DescriptorSetLayout::create(
		std::vector<VkDescriptorSetLayoutBinding> const &bindings
	) {
		auto layout = DescriptorSetLayout();
		layout._bindings = bindings;

		uint32_t i = 0;
		for (auto &b : layout._bindings) {
			if (b.binding == DESCRIPTOR_BINDING_UNUSED) {
				b.binding = i;
			} else if (b.binding != i) {
				return KError::invalid_arg("Invalid binding order");
			}
			i++;
		}

		auto layout_info = VkDescriptorSetLayoutCreateInfo{};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.pBindings = layout._bindings.data();
		layout_info.bindingCount = static_cast<uint32_t>(layout._bindings.size());
		
		auto r = vkCreateDescriptorSetLayout(
			Graphics::DEFAULT->device(),
			&layout_info,
			nullptr,
			&layout._layout
		);

		if (r != VK_SUCCESS) {
			return {r};
		}

		return layout;
	}

	DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout &&other) {
		_bindings = std::move(other._bindings);
		_layout = other._layout;
		other._layout = nullptr;
	}

	DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout &&other) {
		destroy();

		_bindings = std::move(other._bindings);
		_layout = other._layout;
		other._layout = nullptr;

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

	DescriptorSetBuilder DescriptorSetLayout::builder(uint32_t frame_count) {
		return DescriptorSetBuilder(*this, frame_count);
	}

	VkDescriptorSetLayout &DescriptorSetLayout::layout() {
		return _layout;
	}

	std::vector<VkDescriptorSetLayoutBinding> const &DescriptorSetLayout::bindings() const {
		return _bindings;
	}

	DescriptorSetBuilder::DescriptorSetBuilder(
		DescriptorSetLayout &layout,
		uint32_t frame_count
	):
		_layout(&layout),
		_cur_binding(0),
		_frame_count(frame_count),
		_descriptor_writes(layout.bindings().size()),
		_buffer_infos(layout.bindings().size()),
		_image_infos(layout.bindings().size()),
		_buffer_ranges(layout.bindings().size()),
		_image_views(layout.bindings().size()),
		_image_layout(layout.bindings().size())
	{}

	DescriptorSetBuilder::DescriptorSetBuilder(DescriptorSetBuilder &&other) {
		_layout = other._layout;
		other._layout = nullptr;

		_cur_binding = other._cur_binding;

		_descriptor_writes = std::move(other._descriptor_writes);
		_buffer_infos = std::move(other._buffer_infos);
		_image_infos = std::move(other._image_infos);
		_buffer_ranges = std::move(other._buffer_ranges);
		_image_views = std::move(other._image_views);
		_image_layout = std::move(other._image_layout);
	}

	DescriptorSetBuilder &DescriptorSetBuilder::operator=(DescriptorSetBuilder &&other) {
		_layout = other._layout;
		other._layout = nullptr;

		_cur_binding = other._cur_binding;

		_descriptor_writes = std::move(other._descriptor_writes);
		_buffer_infos = std::move(other._buffer_infos);
		_image_infos = std::move(other._image_infos);
		_buffer_ranges = std::move(other._buffer_ranges);
		_image_views = std::move(other._image_views);
		_image_layout = std::move(other._image_layout);

		return *this;
	}

	bool DescriptorSetBuilder::has_value() const { return _layout; }

	DescriptorSetBuilder::operator bool() const { return has_value(); }

	DescriptorSetLayout &DescriptorSetBuilder::layout() {
		return *_layout;
	}

	VkDescriptorSetLayout *DescriptorSetBuilder::vk_layout() {
		return &_layout->layout();
	}

	std::vector<VkWriteDescriptorSet> &DescriptorSetBuilder::writes() {
		return _descriptor_writes;
	}

	uint32_t DescriptorSetBuilder::frame_count() const {
		return _frame_count;
	}

	util::Result<DescriptorSetBuilder &, KError> DescriptorSetBuilder::add_uniform(
		Uniform &uniform
	) {
		return add_uniform({uniform.buffer()}, uniform.size());
	}

	util::Result<DescriptorSetBuilder &, KError> DescriptorSetBuilder::add_uniform(
		std::vector<VkBuffer> const &buffers,
		size_t buffer_size
	) {
		auto &binding = _layout->bindings()[_cur_binding];
		if (binding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
			return KError::invalid_arg(util::f("Binding ", _cur_binding, " is not a uniform buffer"));
		}

		auto &buffer_infos = _buffer_infos[_cur_binding];
		for (auto &buffer : buffers) {
			auto buffer_info = VkDescriptorBufferInfo{};
			
			buffer_info.buffer = buffer;
			buffer_info.offset = 0;
			buffer_info.range = buffer_size;

			buffer_infos.push_back(buffer_info);
		}

		auto &descriptor_write = _descriptor_writes[_cur_binding];
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstBinding = _cur_binding;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_write.pBufferInfo = buffer_infos.data();
		descriptor_write.descriptorCount = buffer_infos.size();

		_cur_binding++;
		return *this;
	}

	util::Result<DescriptorSetBuilder &, KError> DescriptorSetBuilder::add_image(
		VkImageView image_view
	) {
		return add_image(image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	util::Result<DescriptorSetBuilder &, KError> DescriptorSetBuilder::add_image(
		VkImageView image_view,
		VkImageLayout image_layout
	) {
		auto &binding = _layout->bindings()[_cur_binding];
		if (binding.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
			return KError::invalid_arg(util::f("Binding ", _cur_binding, " is not an image sampler"));
		}

		if (image_view == VK_NULL_HANDLE) {
			return KError::invalid_arg("image_view cannot be VK_NULL_HANDLE");
		}

		auto &image_infos = _image_infos[_cur_binding];
		auto image_info = VkDescriptorImageInfo{};
		image_info.imageLayout = image_layout;
		image_info.imageView = image_view;
		image_info.sampler = *Graphics::DEFAULT->main_texture_sampler();
		image_infos.push_back(image_info);

		auto &descriptor_write = _descriptor_writes[_cur_binding];
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstBinding = _cur_binding;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_write.pImageInfo = image_infos.data();
		descriptor_write.descriptorCount = image_infos.size();

		_cur_binding++;
		return *this;
	}

	util::Result<DescriptorSetBuilder &, KError> DescriptorSetBuilder::add_image(
		std::vector<VkImageView> const &image_views
	) {
		auto &binding = _layout->bindings()[_cur_binding];
		if (binding.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
			return KError::invalid_arg(util::f("Binding ", _cur_binding, " is not an image sampler"));
		}
		if (image_views.size() == 0) {
			return KError::invalid_arg("Cannot use 0 images");
		}

		auto &image_infos = _image_infos[_cur_binding];
		
		for (auto &image_view : image_views) {
			if (image_view == VK_NULL_HANDLE) {
				//return KError::invalid_arg("image_views cannot contain VK_NULL_HANDLE");
			}
			auto image_info = VkDescriptorImageInfo{};
			image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_info.imageView = image_view;
			image_info.sampler = *Graphics::DEFAULT->main_texture_sampler();
			image_infos.push_back(image_info);
		}

		auto &descriptor_write = _descriptor_writes[_cur_binding];
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstBinding = _cur_binding;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_write.pImageInfo = image_infos.data();
		descriptor_write.descriptorCount = image_infos.size();

		_cur_binding++;
		return *this;
	}

	util::Result<DescriptorSetBuilder &, KError> DescriptorSetBuilder::add_image_target(
		VkImageView image_view
	) {
		auto &binding = _layout->bindings()[_cur_binding];
		if (binding.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
			return KError::invalid_arg(util::f("Binding ", _cur_binding, " is not an image target"));
		}

		if (image_view == VK_NULL_HANDLE) {
			return KError::invalid_arg("image_view cannot be VK_NULL_HANDLE");
		}

		auto &image_infos = _image_infos[_cur_binding];
		auto image_info = VkDescriptorImageInfo{};
		image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		image_info.imageView = image_view;
		image_info.sampler = *Graphics::DEFAULT->main_texture_sampler();
		image_infos.push_back(image_info);

		auto &descriptor_write = _descriptor_writes[_cur_binding];
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstBinding = _cur_binding;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptor_write.pImageInfo = image_infos.data();
		descriptor_write.descriptorCount = image_infos.size();

		_cur_binding++;
		return *this;
	}

	util::Result<DescriptorSetBuilder &, KError> DescriptorSetBuilder::add_storage_buffer(
		VkBuffer buffer,
		size_t range
	) {
		auto &binding = _layout->bindings()[_cur_binding];
		if (binding.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
			return KError::invalid_arg(util::f("Binding ", _cur_binding, " is not a storage buffer"));
		}


		auto &buffer_infos = _buffer_infos[_cur_binding];
		auto buffer_info = VkDescriptorBufferInfo{};
		buffer_info.buffer = buffer;
		buffer_info.offset = 0;
		buffer_info.range = range;
		buffer_infos.push_back(buffer_info);

		auto &descriptor_write = _descriptor_writes[_cur_binding];
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstBinding = _cur_binding;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptor_write.pBufferInfo = buffer_infos.data();
		descriptor_write.descriptorCount = buffer_infos.size();

		_cur_binding++;
		return *this;
	}

	util::Result<DescriptorSetBuilder &, KError> DescriptorSetBuilder::add_storage_buffer(
		StaticBuffer &static_buffer
	) {
		return add_storage_buffer(
			static_buffer.buffer(),
			static_buffer.range()
		);
	}

	DescriptorSetBuilder &DescriptorSetBuilder::set_sampler(
		uint32_t binding,
		Sampler const &sampler
	) {
		for (auto &image_info : _image_infos[binding]) {
			image_info.sampler = sampler.get();
		}
		return *this;
	}

	uint32_t DescriptorSetBuilder::initialized_bindings() const {
		return _cur_binding;
	}

	util::Result<DescriptorSets, KError> DescriptorSets::create(
		DescriptorSetBuilder &builder,
		DescriptorPool &pool
	) {
		auto result = DescriptorSets();
		result._descriptor_pool = &pool;

		if (builder.initialized_bindings() != builder.layout().bindings().size()) {
			return KError::invalid_arg("Not all bindings in the builder are initialized.");
		}
		
		auto layout_vec = std::vector<VkDescriptorSetLayout>(
			builder.frame_count(),
			*builder.vk_layout()
		);

		auto alloc_info = VkDescriptorSetAllocateInfo{};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = pool.descriptor_pool();
		alloc_info.descriptorSetCount = layout_vec.size();
		alloc_info.pSetLayouts = layout_vec.data();

		result._descriptor_sets.resize(builder.frame_count());
		auto res = vkAllocateDescriptorSets(
			Graphics::DEFAULT->device(),
			&alloc_info,
			result._descriptor_sets.data()
		);
		if (res != VK_SUCCESS) {
			return {res};
		}

		for (size_t frame = 0; frame < builder.frame_count(); frame++) {
			for (auto &write : builder.writes()) {
				//Feels wrong as I am modifying a parameter passed in
				//However, builder is so specialzied rn that I don't
				//think there will be side effects
				write.dstSet = result._descriptor_sets[frame];
			}
			vkUpdateDescriptorSets(
				Graphics::DEFAULT->device(),
				static_cast<uint32_t>(builder.writes().size()),
				builder.writes().data(),
				0,
				nullptr	
			);
		}

		return result;
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

	VkDescriptorSet DescriptorSets::descriptor_set(uint32_t frame_index) {
		return _descriptor_sets[frame_index];
	}
}
