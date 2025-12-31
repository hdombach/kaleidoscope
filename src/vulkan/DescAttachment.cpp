#include "DescAttachment.hpp"

#include "Image.hpp"
#include "DescriptorSet.hpp"

namespace vulkan {
	const char *DescAttachment::type_str(Type type) {
		switch(type) {
			case Type::UNKNOWN:
				return "Unknown";
			case Type::UNIFORM:
				return "Uniform";
			case Type::STORAGE_BUFFER:
				return "storage buffer";
			case Type::IMAGE:
				return "image";
			case Type::IMAGE_TARGET:
				return "image target";
		}
	}

	DescAttachment DescAttachment::create_uniform(VkShaderStageFlags shader_stage) {
		return DescAttachment(Type::UNIFORM, shader_stage);
	}

	DescAttachment DescAttachment::create_storage_buffer(VkShaderStageFlags shader_stage) {
		return DescAttachment(Type::STORAGE_BUFFER, shader_stage);
	}

	DescAttachment DescAttachment::create_image(VkShaderStageFlags shader_stage) {
		return DescAttachment(Type::IMAGE, shader_stage);
	}

	DescAttachment DescAttachment::create_images(
		VkShaderStageFlags shader_stage,
		size_t count
	) {
		auto a = DescAttachment(Type::IMAGE, shader_stage);
		a._descriptor_count = count;
		return a;
	}

	DescAttachment DescAttachment::create_image_target(VkShaderStageFlags shader_stage) {
		return DescAttachment(Type::IMAGE_TARGET, shader_stage);
	}

	DescAttachment &DescAttachment::add_uniform(Uniform &uniform) {
		return add_uniform(uniform.buffer(), uniform.size());
	}

	DescAttachment &DescAttachment::add_uniform(VkBuffer buffer, size_t buffer_size) {
		if (_error) return *this;
		if (_type != Type::UNIFORM) {
			_error = Error(
				ErrorType::SHADER_RESOURCE,
				util::f("Cannot add uniform to attachment of type ", type_str(_type))
			);
			return *this;
		}

		_buffer = buffer;
		_buffer_size = buffer_size;

		return *this;
	}

	DescAttachment &DescAttachment::add_buffer(StaticBuffer &static_buffer) {
		return add_buffer(static_buffer.buffer(), static_buffer.range());
	}

	DescAttachment &DescAttachment::add_buffer(VkBuffer buffer, size_t range) {
		if (_error) return *this;
		if (_type != Type::STORAGE_BUFFER) {
			_error = Error(
				ErrorType::SHADER_RESOURCE,
				util::f("Cannot add buffer to attachment of type ", type_str(_type))
			);
			return *this;
		}
		_buffer = buffer;
		_buffer_size = range;

		return *this;
	}

	DescAttachment &DescAttachment::add_image(Image const &image) {
		return add_images({image.image_view()});
	}

	DescAttachment &DescAttachment::add_images(std::vector<VkImageView> const &image_views) {
		if (_error) return *this;
		if (_type != Type::IMAGE) {
			_error = Error(
				ErrorType::SHADER_RESOURCE,
				util::f("Cannot add images to attachment of type ", type_str(_type))
			);
			return *this;
		}

		if (image_views.size() == 0) {
			_error = Error(
				ErrorType::INVALID_ARG,
				"Cannot add 0 images to an attachment"
			);
			return *this;
		}

		_image_views = image_views;
		_image_layout = VK_IMAGE_LAYOUT_GENERAL;

		_sampler = *Graphics::DEFAULT->main_texture_sampler();

		return *this;
	}

	DescAttachment &DescAttachment::add_image_target(VkImageView image_view) {
		if (_error) return *this;
		if (_type != Type::IMAGE_TARGET) {
			_error = Error(
				ErrorType::SHADER_RESOURCE,
				util::f("Cannot add image target to attachment of type ", type_str(_type))
			);
			return *this;
		}

		_image_views = {image_view};
		_image_layout = VK_IMAGE_LAYOUT_GENERAL;

		return *this;
	}

	DescAttachment &DescAttachment::set_sampler(Sampler const &sampler) {
		if (_error) return *this;
		if (_type != Type::IMAGE) {
			_error = Error(
				ErrorType::SHADER_RESOURCE,
				util::f("Cannot assign a sampler to attachment of type ", type_str(_type))
			);
			return *this;
		}

		_sampler = sampler.get();
		return *this;
	}

	DescAttachment &DescAttachment::set_image_layout(VkImageLayout layout) {
		if (_error) return *this;
		if (_type != Type::IMAGE) {
			_error = Error(
				ErrorType::SHADER_RESOURCE,
				util::f("Cannot assign image layout to attachment of type ", type_str(_type))
			);
			return *this;
		}

		_image_layout = layout;
		return *this;
	}

	util::Result<VkDescriptorSetLayoutBinding, Error> DescAttachment::descriptor_binding() const {
		if (_error) return _error.value();

		auto binding = VkDescriptorSetLayoutBinding{};
		binding.descriptorCount = _descriptor_count;

		switch (_type) {
			case Type::UNIFORM:
				binding.binding = DESCRIPTOR_BINDING_UNUSED;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				binding.stageFlags = _shader_stage;
				binding.pImmutableSamplers = nullptr;
				break;
			case Type::STORAGE_BUFFER:
				binding.binding = DESCRIPTOR_BINDING_UNUSED;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				binding.stageFlags = _shader_stage;
				binding.pImmutableSamplers = nullptr;
				break;
			case Type::IMAGE:
				binding.binding = DESCRIPTOR_BINDING_UNUSED;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				binding.stageFlags = _shader_stage;
				binding.pImmutableSamplers = nullptr;
				break;
			case Type::IMAGE_TARGET:
				binding.binding = DESCRIPTOR_BINDING_UNUSED;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				binding.stageFlags = _shader_stage;
				binding.pImmutableSamplers = nullptr;
				break;
			case Type::UNKNOWN:
				return Error(
					ErrorType::SHADER_RESOURCE,
					"Can't create binding for attachment without a type"
				);
		}

		return binding;
	}

	util::Result<VkWriteDescriptorSet, Error> DescAttachment::descriptor_write() {
		if (_error) return _error.value();

		auto descriptor_write = VkWriteDescriptorSet{};
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.descriptorCount = _descriptor_count;

		int i;
		switch (_type) {
			case Type::UNIFORM:
				if (_buffer == nullptr) {
					return Error(
						ErrorType::SHADER_RESOURCE,
						"Must attach a buffer before resolving the descriptor_write"
					);
				}

				_buffer_info.buffer = _buffer;
				_buffer_info.offset = 0;
				_buffer_info.range = _buffer_size;

				descriptor_write.dstBinding = DESCRIPTOR_BINDING_UNUSED;
				descriptor_write.dstArrayElement = 0;
				descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptor_write.pBufferInfo = &_buffer_info;
				break;
			case Type::STORAGE_BUFFER:
				if (_buffer == nullptr) {
					return Error(
						ErrorType::SHADER_RESOURCE,
						"Must attach a buffer before resolving the descriptor_write"
					);
				}

				_buffer_info.buffer = _buffer;
				_buffer_info.offset = 0;
				_buffer_info.range = _buffer_size;

				descriptor_write.dstBinding = DESCRIPTOR_BINDING_UNUSED;
				descriptor_write.dstArrayElement = 0;
				descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptor_write.pBufferInfo = &_buffer_info;
				break;
			case Type::IMAGE:
				if (_image_views.empty()) {
					return Error(
						ErrorType::SHADER_RESOURCE,
						"Must attach an image before resolving the descriptor write"
					);
				}
				_image_infos.resize(_image_views.size());
				i = 0;
				for (auto &image_view : _image_views) {
					auto &image_info = _image_infos[i];

					image_info.imageLayout = _image_layout;
					image_info.imageView = image_view;
					image_info.sampler = _sampler;
					i++;
				}

				descriptor_write.dstBinding = DESCRIPTOR_BINDING_UNUSED;
				descriptor_write.dstArrayElement = 0;
				descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptor_write.pImageInfo = _image_infos.data();
				break;
			case Type::IMAGE_TARGET:
				if (_image_views.empty()) {
					return Error(
						ErrorType::SHADER_RESOURCE,
						"Must attach an image before resolving the descriptor write"
					);
				}
				if (_image_views[0] == nullptr) {
					return Error(
						ErrorType::INVALID_ARG,
						"Cannot attach a nullptr image view"
					);
				}

				log_assert(_image_views.size() == 1, "There must be 1 image view");
				_image_infos.resize(1);
				_image_infos[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				_image_infos[0].imageView = _image_views[0];
				_image_infos[0].sampler = _sampler;

				descriptor_write.dstBinding = DESCRIPTOR_BINDING_UNUSED;
				descriptor_write.dstArrayElement = 0;
				descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				descriptor_write.pImageInfo = _image_infos.data();
				break;
			default:
				return Error(
					ErrorType::SHADER_RESOURCE,
					"Cannot create descriptor write for attachment without a type"
				);
		}

		return descriptor_write;
	}

	DescAttachment::DescAttachment(Type type, VkShaderStageFlags shader_stage):
		_type(type),
		_shader_stage(shader_stage)
	{ }
}

std::ostream &operator<<(std::ostream &os, vulkan::DescAttachment::Type const &type) {
	return os << vulkan::DescAttachment::type_str(type);
}
