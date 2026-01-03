#include "DescriptorSet.hpp"

#include <vulkan/vulkan_core.h>

namespace vulkan {
	using util::f;

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
		std::vector<VkDescriptorSetLayoutBinding> const &bindings
	) {
		auto layout = DescriptorSetLayout();
		layout._bindings = bindings;

		uint32_t i = 0;
		for (auto &b : layout._bindings) {
			if (b.binding == DESCRIPTOR_BINDING_UNUSED) {
				b.binding = i;
			} else if (b.binding != i) {
				return Error(ErrorType::INVALID_ARG, "Invalid binding order");
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
			return Error(ErrorType::INVALID_ARG, "Could not create descriptor set layout", VkError(r));
		}

		return layout;
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
		_bindings = std::move(other._bindings);
		_layout = util::move_ptr(other._layout);
		_desc_attachments = std::move(other._desc_attachments);
	}

	DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout &&other) {
		destroy();

		_bindings = std::move(other._bindings);
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

	DescriptorSetBuilder DescriptorSetLayout::builder(uint32_t frame_count) const {
		return DescriptorSetBuilder(*this, frame_count);
	}

	VkDescriptorSetLayout const &DescriptorSetLayout::layout() const {
		return _layout;
	}

	std::vector<VkDescriptorSetLayoutBinding> const &DescriptorSetLayout::bindings() const {
		return _bindings;
	}

	std::vector<DescAttachment> const &DescriptorSetLayout::desc_attachments() const {
		return _desc_attachments;
	}

	DescriptorSetBuilder::DescriptorSetBuilder(
		DescriptorSetLayout const &layout,
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

	DescriptorSetLayout const &DescriptorSetBuilder::layout() const {
		return *_layout;
	}

	VkDescriptorSetLayout const *DescriptorSetBuilder::vk_layout() const {
		return &_layout->layout();
	}

	std::vector<VkWriteDescriptorSet> const &DescriptorSetBuilder::writes() const {
		return _descriptor_writes;
	}

	uint32_t DescriptorSetBuilder::frame_count() const {
		return _frame_count;
	}

	util::Result<void, Error> DescriptorSetBuilder::add_uniform(
		Uniform &uniform
	) {
		return add_uniform({uniform.buffer()}, uniform.size());
	}

	util::Result<void, Error> DescriptorSetBuilder::add_uniform(
		std::vector<VkBuffer> const &buffers,
		size_t buffer_size
	) {
		auto &binding = _layout->bindings()[_cur_binding];
		if (binding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
			auto msg = f(
				"Binding ", _cur_binding, " is not a uniform buffer. (expecting ",
				descriptor_type_str(binding.descriptorType), ")"
			);
			return Error(ErrorType::INVALID_ARG, msg);
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
		return {};
	}

	util::Result<void, Error> DescriptorSetBuilder::add_image(
		VkImageView image_view
	) {
		return add_image(image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	util::Result<void, Error> DescriptorSetBuilder::add_image(
		VkImageView image_view,
		VkImageLayout image_layout
	) {
		auto &binding = _layout->bindings()[_cur_binding];
		if (binding.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
			auto msg = f(
				"Binding ", _cur_binding, " is not an image sampler. (expecting ",
				descriptor_type_str(binding.descriptorType), ")"
			);
			return Error(ErrorType::INVALID_ARG, msg);
		}

		if (image_view == VK_NULL_HANDLE) {
			return Error(ErrorType::INVALID_ARG, "image_view cannot be VK_NULL_HANDLE");
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
		return {};
	}

	util::Result<void, Error> DescriptorSetBuilder::add_image(
		std::vector<VkImageView> const &image_views
	) {
		auto &binding = _layout->bindings()[_cur_binding];
		if (binding.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
			auto msg = f(
				"Binding ", _cur_binding, " is not an image sampler. (epxecting ",
				descriptor_type_str(binding.descriptorType), ")"
			);
			return Error(ErrorType::INVALID_ARG, msg);
		}
		if (image_views.size() == 0) {
			return Error(ErrorType::INVALID_ARG, "Cannot use 0 images");
		}

		auto &image_infos = _image_infos[_cur_binding];
		
		for (auto &image_view : image_views) {
			if (image_view == VK_NULL_HANDLE) {
				//return Error(ErrorType::INVALID_ARG, "image_views cannot contain VK_NULL_HANDLE");
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
		return {};
	}

	util::Result<void, Error> DescriptorSetBuilder::add_image_target(
		VkImageView image_view
	) {
		auto &binding = _layout->bindings()[_cur_binding];
		if (binding.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
			auto msg = f(
				"Binding ", _cur_binding, " is not an image target. (expecting ",
				descriptor_type_str(binding.descriptorType), ")"
			);
			return Error(ErrorType::INVALID_ARG, msg);
		}

		if (image_view == VK_NULL_HANDLE) {
			return Error(ErrorType::INVALID_ARG, "image_view cannot be VK_NULL_HANDLE");
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
		return {};
	}

	util::Result<void, Error> DescriptorSetBuilder::add_storage_buffer(
		VkBuffer buffer,
		size_t range
	) {
		auto &binding = _layout->bindings()[_cur_binding];
		if (binding.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
			auto msg = f(
				"Binding ", _cur_binding, " is not a storage buffer. (epxecting ",
				descriptor_type_str(binding.descriptorType), ")"
			);
			return Error(ErrorType::INVALID_ARG, msg);
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
		return {};
	}

	util::Result<void, Error> DescriptorSetBuilder::add_storage_buffer(
		StaticBuffer &static_buffer
	) {
		return add_storage_buffer(
			static_buffer.buffer(),
			static_buffer.range()
		);
	}

	void DescriptorSetBuilder::set_sampler(
		uint32_t binding,
		Sampler const &sampler
	) {
		for (auto &image_info : _image_infos[binding]) {
			image_info.sampler = sampler.get();
		}
	}

	uint32_t DescriptorSetBuilder::initialized_bindings() const {
		return _cur_binding;
	}

	util::Result<DescriptorSets, Error> DescriptorSets::create(
		DescriptorSetBuilder &builder,
		DescriptorPool const &pool
	) {
		auto result = DescriptorSets();
		result._descriptor_pool = &pool;

		if (builder.initialized_bindings() != builder.layout().bindings().size()) {
			return Error(ErrorType::INVALID_ARG, "Not all bindings in the builder are initialized");
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
			return Error(ErrorType::VULKAN, "Cannot allocate descriptor sets", VkError(res));
		}

		for (size_t frame = 0; frame < builder.frame_count(); frame++) {
			auto writes = builder.writes();
			for (auto &write : writes) {
				//Feels wrong as I am modifying a parameter passed in
				//However, builder is so specialzied rn that I don't
				//think there will be side effects
				write.dstSet = result._descriptor_sets[frame];
			}
			vkUpdateDescriptorSets(
				Graphics::DEFAULT->device(),
				static_cast<uint32_t>(writes.size()),
				writes.data(),
				0,
				nullptr	
			);
		}

		return result;
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
