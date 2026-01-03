#pragma once

#include <vulkan/vulkan_core.h>

#include "Uniforms.hpp"
#include "StaticBuffer.hpp"

namespace vulkan {
	class Image;

	/**
	 * @brief A description of resources that are attached to vulkan shader
	 * descriptor sets.
	 *
	 * Since the attachment can be used with both DescriptorSets and
	 * DescriptorSetLayout, the attacment can have either a template
	 * or filled state.
	 *
	 * This class is not to be confused with a ShaderResource. The ShaderResource
	 * abstracts away the vulkan implimentation and allows users to set
	 * primitives or textures direclty. The ShaderResource can also support
	 * multiple different layouts of primitives in the same RenderPass.
	 * Attachments meanwhile have the same level of strickness that vulkan has.
	 */
	class DescAttachment {
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
			DescAttachment() = default;

			/**
			 * @brief Describes a uniform
			 */
			static DescAttachment create_uniform(
				VkShaderStageFlags shader_stage
			);

			/**
			 * @brief Describes a general buffer
			 */
			static DescAttachment create_storage_buffer(
				VkShaderStageFlags shader_stage
			);

			/**
			 * @brief Creates an image sampler
			 */
			static DescAttachment create_image(
				VkShaderStageFlags shader_stage
			);

			/**
			 * @brief Create an image sampler for a collection of textures
			 */
			static DescAttachment create_images(
				VkShaderStageFlags shader_stage,
				size_t count
			);

			/**
			 * @brief Creates an image that can be written too arbitrarily
			 */
			static DescAttachment create_image_target(
				VkShaderStageFlags shader_stage
			);

			/**
			 * @brief Attaches a mapped uniform
			 */
			template<typename BufferObj>
			DescAttachment &add_uniform(MappedUniform<BufferObj> &mapped_uniform) {
				return add_uniform(mapped_uniform.buffer(), sizeof(BufferObj));
			}

			/**
			 * @brief Attaches a uniform
			 */
			DescAttachment &add_uniform(Uniform &uniform);

			/**
			 * @brief Attaches a raw buffer as a uniform
			 */
			DescAttachment &add_uniform(VkBuffer buffer, size_t buffer_size);

			/**
			 * @brief Attaches a general use buffer
			 */
			DescAttachment &add_buffer(StaticBuffer &static_buffer);

			/**
			 * @brief Attaches a raw vulkan buffer
			 */
			DescAttachment &add_buffer(VkBuffer buffer, size_t range);

			/**
			 * @brief Attaches an image
			 */
			DescAttachment &add_image(Image const &Image);

			/**
			 * @brief Attaches multiple image views
			 */
			DescAttachment &add_images(std::vector<VkImageView> const &image_views);

			/**
			 * @brief Attaches an image target
			 */
			DescAttachment &add_image_target(VkImageView image_view);

			/**
			 * @brief Set a different sampler from the default
			 *
			 * Can only be used with image attachment types
			 */
			DescAttachment &set_sampler(Sampler const &sampler);

			/**
			 * @brief Sets the image layout
			 *
			 * Can only be used with image attachment types
			 */
			DescAttachment &set_image_layout(VkImageLayout layout);

			/**
			 * @brief Resolves the descriptor binding
			 *
			 * Will throw an error if there isn't enough information provided
			 * or contradictory information is provided
			 *
			 * The binding index needs to be handeled by the caller
			 */
			util::Result<VkDescriptorSetLayoutBinding, Error> descriptor_binding() const;

			/**
			 * @brief Resolves the VkWriteDescriptorSet
			 *
			 * Will throw an error if there isn't enough information provided or
			 * contradictory information is provided
			 *
			 * The binding index needs to be handeled by the caller.
			 * VkWriteDescriptorSet contains points to bojects stored by DescAttachment.
			 */
			util::Result<VkWriteDescriptorSet, Error> descriptor_write();

			/**
			 * @brief Get a debug description of the os
			 */
			std::ostream &print_debug(std::ostream &os) const;

		private:
			/**
			 * Internally keeps track of any error that occurs
			 *
			 * This alows modifiers to be chained together easily. The error only
			 * appears when trying to resolve the builder structs
			 */
			std::optional<Error> _error = std::nullopt;
			Type _type;

			VkFormat _image_format = VK_FORMAT_UNDEFINED;
			VkShaderStageFlags _shader_stage;
			uint32_t _descriptor_count = 1; // currently used for multiple images

			std::vector<VkImageView> _image_views = {};
			VkImageLayout _image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkSampler _sampler;

			VkBuffer _buffer = nullptr;
			size_t _buffer_size = 0;

			// Helper constructor classes
			VkDescriptorBufferInfo _buffer_info;
			std::vector<VkDescriptorImageInfo> _image_infos;

		private:
			DescAttachment(Type type, VkShaderStageFlags shader_stage);
	};
}

std::ostream &operator<<(std::ostream &os, vulkan::DescAttachment::Type const &type);

std::ostream &operator<<(std::ostream &os, vulkan::DescAttachment const &attachment);
