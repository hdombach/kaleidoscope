#pragma once

#include <optional>
#include <vulkan/vulkan_core.h>

#include "util/result.hpp"
#include "Uniforms.hpp"

namespace vulkan {
	class Uniform;
	class Sampler;
	class Image;
	class StaticBuffer;

	/**
	 * @brief A universal description for describing resources that are attached
	 * to a vulkan shader.
	 *
	 * This attachment can be used across descriptor sets, pipelines, and render
	 * passes. To facilitate this, the Attachment can be in either a template or
	 * a filled state. When building the pipeline and renderpasses, checks can
	 * then be done accross Attachments to make sure they are compatible.
	 *
	 * This class is not to be confused with a ShaderResource. The ShaderResource
	 * abstracts away the vulkan implimentation and allows users to set
	 * primitives or textures direclty. The ShaderResource can also support
	 * multiple different layouts of primitives in the same RenderPass.
	 * Attachments meanwhile have the same level of strickness that vulkan has.
	 */
	class Attachment {
		public:
			enum class Type {
				UNKNOWN,
				UNIFORM,
				STORAGE_BUFFER,
				IMAGE,
				IMAGE_TARGET,
			};

			static const char *type_str(Type type);

		public:
			Attachment() = default;

			/**
			 * @brief Describes a uniform
			 */
			static Attachment create_uniform(
				VkShaderStageFlagBits shader_stage
			);

			/**
			 * @brief Describes a general buffer
			 */
			static Attachment create_storage_buffer(
				VkShaderStageFlagBits shader_stage
			);

			/**
			 * @brief Create an image sampler
			 *
			 * Can described a read only texture to sample or a render target in a
			 * framebuffer
			 */
			static Attachment create_image(
				VkShaderStageFlagBits shader_stage
			);

			/**
			 * @brief Create an image sampler for a collection of textures
			 */
			static Attachment create_images(
				VkShaderStageFlagBits shader_stage,
				size_t count
			);

			/**
			 * @brief Creates an image that can be written to arbitrarily
			 */
			static Attachment create_image_target(
				VkShaderStageFlagBits shader_stage
			);

			/**
			 * @brief Attaches a mapped uniform
			 */
			template<typename BufferObj>
			Attachment &add_uniform(MappedUniform<BufferObj> &mapped_unform) {
				return add_uniform(mapped_unform.buffer(), sizeof(BufferObj));
			}

			/**
			 * @brief Attaches a uniform
			 */
			Attachment &add_uniform(Uniform &uniform);
			
			/**
			 * @brief Attach a raw buffer as a uniform
			 */
			Attachment &add_uniform(VkBuffer buffer, size_t buffer_size);

			/**
			 * @brief Attaches a general use buffer
			 */
			Attachment &add_buffer(StaticBuffer &static_buffer);

			/**
			 * @brief Attaches a raw vulkan buffer
			 */
			Attachment &add_buffer(VkBuffer buffer, size_t range);

			/**
			 * @brief Attaches an image
			 */
			Attachment &add_image(Image const &image);

			/**
			 * @brief Attaches multiple image views
			 *
			 * Cannot be used as a framebuffer destination
			 */
			Attachment &add_images(std::vector<VkImageView> const &image_views);

			/**
			 * @brief Attaches an image target
			 */
			Attachment &add_image_target(VkImageView image_view);

			/**
			 * @brief Marks that the attachment is used in the framebuffer
			 *
			 * Can only be used with image attachment types
			 */
			Attachment &set_framebuffer_target(bool framebuffer_target=true);

			/**
			 * @brief Assigns a clear value
			 *
			 * Can only be used with images that are part of a framebuffer.
			 */
			Attachment &set_clear_value(VkClearValue const &clear_value);

			/**
			 * @brief Set a different sampler fromt he default
			 *
			 * Can only be used with image attachment types
			 */
			Attachment &set_sampler(Sampler const &sampler);

			/**
			 * @brief Sets the image layout
			 *
			 * Can only be used with image attachment types
			 */
			Attachment &set_image_layout(VkImageLayout _layout);

			/**
			 * @brief Resolves the descriptor binding
			 *
			 * Will throw an error if there isn't enough information provided
			 * or contradictory information is provided
			 *
			 * The binding index needs to be handeled by the caller
			 */
			util::Result<VkDescriptorSetLayoutBinding, Error> descriptor_binding();

			/**
			 * @brief Resolves the VkWriteDescriptorSet
			 *
			 * Will throw an error if there isn't enough information provided or
			 * contradictory information is provided
			 *
			 * The binding index needs to be handeled by the caller
			 * VkWriteDescriptorSet contains pointers to objects stored by Attachment
			 */
			util::Result<VkWriteDescriptorSet, Error> descriptor_write();

			/**
			 * @brief Resolves the VkAttachmentDescription for the render pass
			 *
			 * Will throw an error if there isn't enough information provided
			 * or contradictory information is provided
			 */
			util::Result<VkAttachmentDescription, Error> attachment_description();

			/**
			 * @brief Resolves the VkPipelineColorBlendAttachmentState for the render pass
			 *
			 * Will throw an error if there isn't enough information provided or
			 * contradictory information is provided
			 */
			util::Result<VkPipelineColorBlendAttachmentState, Error> blend_attachment_state();
		private:
			Type _type;

			/*
			 * Internally keep track of any error that occurs.
			 *
			 * This allows modifiers to be chained together easily. The error only
			 * appears when trying to resolve the builder structs
			 */
			std::optional<Error> _error = std::nullopt;

			VkFormat _image_format = VK_FORMAT_UNDEFINED;
			bool _framebuffer = false;
			VkShaderStageFlagBits _shader_stage;
			uint32_t _descriptor_count = 1; //currently used for multiple images

			//  values
			std::vector<VkImageView> _image_views = {};
			VkImageLayout _image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkClearValue _clear_color;
			VkSampler _sampler;
			bool _is_depth;

			VkBuffer _buffer = nullptr;
			size_t _buffer_size = 0;

			// Constructor classes
			VkDescriptorSetLayoutBinding _descriptor_binding;
			VkWriteDescriptorSet _descriptor_write;
			VkDescriptorBufferInfo _buffer_info;
			std::vector<VkDescriptorImageInfo> _image_infos;

			VkAttachmentDescription _attachment_description;
			VkPipelineColorBlendAttachmentState _blend_attachment_state;
	};

}

std::ostream &operator <<(std::ostream &os, vulkan::Attachment::Type const &type);
