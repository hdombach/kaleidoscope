#include "Attachment.hpp"

#include <vulkan/vulkan_core.h>

#include "vulkan/DescriptorSet.hpp"
#include "util/log.hpp"
#include "util/format.hpp"

namespace vulkan {
	const char *Attachment::type_str(Type type) {
		switch (type) {
			case Type::UNKNOWN:
				return "Unknown";
			case Type::UNIFORM:
				return "Uniform";
			case Type::STORAGE_BUFFER:
				return "Storage buffer";
			case Type::IMAGE:
				return "Image";
			case Type::IMAGE_TARGET:
					return "Image target";
		}
	}

	Attachment Attachment::create_uniform(VkShaderStageFlagBits shader_stage) {
		auto a = Attachment();

		a._type = Type::UNIFORM;
		a._shader_stage = shader_stage;

		return a;
	}

	Attachment Attachment::create_storage_buffer(
		VkShaderStageFlagBits shader_stage
	) {
		auto a = Attachment();

		a._type = Type::STORAGE_BUFFER;
		a._shader_stage = shader_stage;

		return a;
	}

	Attachment Attachment::create_image(
		VkShaderStageFlagBits shader_stage
	) {
		auto a = Attachment();

		a._type = Type::IMAGE;
		a._shader_stage = shader_stage;

		return a;
	}

	Attachment Attachment::create_images(
		VkShaderStageFlagBits shader_stage,
		size_t count
	) {
		auto a = Attachment();

		a._type = Type::IMAGE;
		a._shader_stage = shader_stage;
		a._descriptor_count = count;

		return a;
	}

	Attachment Attachment::create_image_target(
		VkShaderStageFlagBits shader_stage
	) {
		auto a = Attachment();

		a._type = Type::IMAGE_TARGET;
		a._shader_stage = shader_stage;

		return a;
	}

	Attachment &Attachment::add_uniform(Uniform &uniform) {
		return add_uniform(uniform.buffer(), uniform.size());
	}

	Attachment &Attachment::add_uniform(VkBuffer buffer, size_t buffer_size) {
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

	Attachment &Attachment::add_buffer(StaticBuffer &static_buffer) {
		return add_buffer(static_buffer.buffer(), static_buffer.range());
	}

	Attachment &Attachment::add_buffer(VkBuffer buffer, size_t range) {
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

	Attachment &Attachment::add_image(Image const &image) {
		if (_error) return *this;
		if (_type != Type::IMAGE) {
			_error = Error(
				ErrorType::SHADER_RESOURCE,
				util::f("Cannot add image to attachment of type ", type_str(_type))
			);
			return *this;
		}
		_image_format = image.format();
		_image_views = {image.image_view()};
		_image_layout = VK_IMAGE_LAYOUT_GENERAL;

		//clear color used by framebuffer
		switch (_image_format) {
			case VK_FORMAT_D32_SFLOAT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
			case VK_FORMAT_D24_UNORM_S8_UINT:
				_is_depth = true;
				_clear_color = {1.0f, 0};
				break;
			case VK_FORMAT_R16_UINT:
				_clear_color = {0};
				break;
			case VK_FORMAT_R8_SRGB:
				_clear_color = {0};
				break;
			case VK_FORMAT_R32G32_SFLOAT:
				_clear_color = {0.0, 0.0};
				break;
			case VK_FORMAT_R32G32B32_SFLOAT:
				_clear_color = {0.0, 0.0, 0.0};
				break;
			case VK_FORMAT_R32G32B32A32_SFLOAT:
				_clear_color = {0.0, 0.0, 0.0, 1.0};
				break;
			case VK_FORMAT_R8G8B8A8_UNORM:
				_clear_color = {0, 0, 0, 1};
				break;
			default:
				log_error() << "Unimplimented image format: " << _image_format << std::endl;
				break;
		}

		_sampler = *Graphics::DEFAULT->main_texture_sampler();

		return *this;
	}

	Attachment &Attachment::add_images(std::vector<VkImageView> const &image_views) {
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

		//Image format not set

		_image_views = image_views;
		_image_layout  = VK_IMAGE_LAYOUT_GENERAL;
		_is_depth = false;

		//clear color not set

		_sampler = *Graphics::DEFAULT->main_texture_sampler();

		return *this;
	}

	Attachment &Attachment::add_image_target(VkImageView image_view) {
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

	Attachment &Attachment::set_framebuffer_target(bool framebuffer_target) {
		if (_error) return *this;
		if (_type != Type::IMAGE) {
			_error = Error(
				ErrorType::SHADER_RESOURCE,
				util::f("Cannot mark attachment of type ", type_str(_type), " to be used by the framebuffer")
			);
			return *this;
		}

		if (_image_views.size() != 1) {
			_error = Error(
				ErrorType::SHADER_RESOURCE,
				"Can only set framebuffer target for attachment with one image"
			);
		}

		_framebuffer = true;
		return *this;
	}

	Attachment &Attachment::set_clear_value(VkClearValue const &clear_value) {
		if (_error) return *this;
		if (_type != Type::IMAGE) {
			_error = Error(
				ErrorType::SHADER_RESOURCE,
				util::f("Cannot assign a clear value to attachment of type ", type_str(_type))
			);
			return *this;
		}

		_clear_color = clear_value;
		return *this;
	}

	Attachment &Attachment::set_sampler(Sampler const &sampler) {
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

	Attachment &Attachment::set_image_layout(VkImageLayout layout) {
		if (_error) return *this;
		if (_type != Type::IMAGE) {
			_error = Error(
				ErrorType::SHADER_RESOURCE,
				util::f("Cannot assigne image layout to attachment of type ", type_str(_type))
			);
			return *this;
		}

		_image_layout = layout;
		return *this;
	}

	util::Result<VkDescriptorSetLayoutBinding, Error> Attachment::descriptor_binding() {
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

	util::Result<VkWriteDescriptorSet, Error> Attachment::descriptor_write() {
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

	util::Result<VkAttachmentDescription, Error> Attachment::attachment_description() {
		return Error(ErrorType::INTERNAL, "Unimplimented");
	}

	util::Result<VkPipelineColorBlendAttachmentState, Error> Attachment::blend_attachment_state() {
		return Error(ErrorType::INTERNAL, "Unimplimented");
	}
}

std::ostream &operator <<(std::ostream &os, vulkan::Attachment::Type const &type) {
	return os << vulkan::Attachment::type_str(type);
}
