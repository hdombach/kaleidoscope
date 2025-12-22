#pragma once

#include <cstdint>
#include <limits>

#include <vulkan/vulkan_core.h>

#include "util/result.hpp"
#include "DescriptorPool.hpp"
#include "MappedUniform.hpp"
#include "StaticBuffer.hpp"
#include "Error.hpp"

/**
 * @file Tools for building and storing descriptor sets
 */

namespace vulkan {
	static const uint32_t DESCRIPTOR_BINDING_UNUSED = std::numeric_limits<uint32_t>::max();

	/**
	 * @brief Attach a uniform object
	 * @param[in] starge_flags What shader stages this resource will be used in
	 */
	VkDescriptorSetLayoutBinding descriptor_layout_uniform(
		VkShaderStageFlags stage_flags
	);

	/**
	 * @brief Attach a single read only image sampler
	 * @param[in] stage_flags What shader stages this resource will be used in
	 */
	VkDescriptorSetLayoutBinding descriptor_layout_image(VkShaderStageFlags stage_flags);

	/**
	 * @brief Attach one or more read only image samplers
	 * @param[in] stage_flags What shader stages this resource will be used in
	 * @param[in] image_count
	 */
	VkDescriptorSetLayoutBinding descriptor_layout_images(
		VkShaderStageFlags stage_flags,
		size_t image_count
	);

	/**
	 * @brief Attach an image that can be written to.
	 * @param[in] stage_flags What shader stages this resource will be used in
	 */
	VkDescriptorSetLayoutBinding descriptor_layout_image_target(
		VkShaderStageFlags stage_flags,
		size_t image_count
	);

	/**
	 * @brief Attach an abritrary buffer
	 * @param[in] stage_flags What shader stages this resource will be used in
	 */
	VkDescriptorSetLayoutBinding descriptor_layout_storage_buffer(
		VkShaderStageFlags stage_flags
	);

	/**
	 * @brief String representation of the descriptor type
	 */
	const char *descriptor_type_str(VkDescriptorType const &t);

	class DescriptorSetBuilder;

	/**
	 * @brief Describes what types are used in a pipeline along with their order
	 */
	class DescriptorSetLayout {
		public:
			DescriptorSetLayout() = default;

			/**
			 * @brief Creates a new descriptor set layout based off the list of bindings
			 * @param[in] bindings
			 */
			static util::Result<DescriptorSetLayout, Error> create(
				std::vector<VkDescriptorSetLayoutBinding> const &bindings
			);

			DescriptorSetLayout(DescriptorSetLayout const &other) = delete;
			DescriptorSetLayout(DescriptorSetLayout &&other);
			DescriptorSetLayout& operator=(DescriptorSetLayout const &other) = delete;
			DescriptorSetLayout& operator=(DescriptorSetLayout &&other);

			void destroy();
			~DescriptorSetLayout();

			/**
			 * @brief Is the DescriptorSetLayout initialized
			 */
			bool has_value() const;
			/**
			 * @brief Is the DescriptorSetLayout initialized
			 */
			operator bool() const;

			/**
			 * @brief Creates a new builder object for creating a DescriptorSet instance
			 */
			DescriptorSetBuilder builder(uint32_t frame_count = 1) const;

			/**
			 * @brief The core vulkan layout
			 */
			VkDescriptorSetLayout const &layout() const;

			/**
			 * @brief The underlying set of bindings used
			 */
			std::vector<VkDescriptorSetLayoutBinding> const &bindings() const;

		private:
			VkDescriptorSetLayout _layout = nullptr;
			std::vector<VkDescriptorSetLayoutBinding> _bindings;
	};

	/**
	 * @brief The tool for creating a descriptor set with a descriptor set layout
	 * and a list of resources
	 *
	 * Does validation checks to make sure the resources attached are the same type
	 * as describind in the layout
	 *
	 * The resources need to be added in the order that they are expected
	 */
	class DescriptorSetBuilder {
		public:
			/**
			 * @brief Creates an uninitialized DescriptorSetBuilder
			 */
			DescriptorSetBuilder() = default;
			/**
			 * @brief Creates a DescriptorSetBuilder with based on a DescriptorSetLayout
			 */
			DescriptorSetBuilder(DescriptorSetLayout const &layout, uint32_t frame_count);

			//Internal points need to remain valid
			DescriptorSetBuilder(DescriptorSetBuilder const &other) = delete;
			DescriptorSetBuilder(DescriptorSetBuilder &&other);
			DescriptorSetBuilder &operator=(DescriptorSetBuilder const &other) = delete;
			DescriptorSetBuilder &operator=(DescriptorSetBuilder &&other);

			/**
			 * @brief Is the DescriptorSetBuilder initialized
			 */
			bool has_value() const;
			/**
			 * @brief Is the DescriptorSetBuilder initialized
			 */
			operator bool() const;

			/**
			 * @brief The underlying layout used
			 */
			DescriptorSetLayout const &layout() const;
			/**
			 * @brief The underlying vulkan layout used
			 */
			VkDescriptorSetLayout const *vk_layout() const;

			/**
			 * @brief The write objects used when updating the created descriptor set
			 *
			 * The write objects are only partially completed and still need references
			 * to the created DescriptorSet before using it.
			 */
			std::vector<VkWriteDescriptorSet> const &writes() const;

			/**
			 * @brief The number of frames (the number of instances of the descriptor
			 * set which will be created)
			 */
			uint32_t frame_count() const;

			/**
			 * @brief Attaches a list of mapped uniforms
			 * @param[in] mapped_uniforms
			 */
			template<typename BufferObj>
			util::Result<void, Error> add_uniform(
				std::vector<MappedUniform<BufferObj>> &mapped_uniforms
			) {
				auto buffers = std::vector<VkBuffer>();
				for (auto &uniform : mapped_uniforms) {
					buffers.push_back(uniform.buffer());
				}

				return add_uniform(std::move(buffers), sizeof(BufferObj));
			}

			/**
			 * @brief Attaches a maped uniform
			 * @param[in] mapped_uniform
			 */
			template<typename BufferObj>
			util::Result<void, Error> add_uniform(
				MappedUniform<BufferObj> &mapped_uniform
			) {
				return add_uniform({mapped_uniform});
			}

			/**
			 * @brief Attaches a raw uniform
			 * @param[in] uniform
			 */
			util::Result<void, Error> add_uniform(
				Uniform &uniform
			);

			/**
			 * @brief Attaches a raw uniform
			 * @param[in] buffers
			 * @param[in] buffer_size The size of the individual buffers
			 */
			util::Result<void, Error> add_uniform(
				std::vector<VkBuffer> const &buffers,
				size_t buffer_size
			);

			/**
			 * @brief Attaches an image for read only sampling
			 * @param[in] image_view
			 */
			util::Result<void, Error> add_image(
				VkImageView image_view
			);

			/**
			 * @brief Attaches an image with a specified format
			 * @param[in] image_view
			 * @param[in] image_layout
			 */
			util::Result<void, Error> add_image(
				VkImageView image_view,
				VkImageLayout image_layout
			);

			/**
			 * @brief Attaches a list of read only sampled images
			 * @param[in] image_views
			 */
			util::Result<void, Error> add_image(
				std::vector<VkImageView> const &image_views
			);

			/**
			 * @brief Attaches an image to be written too
			 * @param[in] image_view
			 */
			util::Result<void, Error> add_image_target(
				VkImageView image_view
			);

			/**
			 * @brief Attaches an arbitrary buffer of data
			 */
			util::Result<void, Error> add_storage_buffer(
				VkBuffer buffer,
				size_t range
			);

			/**
			 * @brief Attaches an abritrary buffer of data
			 */
			util::Result<void, Error> add_storage_buffer(
				StaticBuffer &static_buffer
			);

			/**
			 * @brief Set the sampler type for a specific image
			 */
			void set_sampler(uint32_t binding, Sampler const &sampler);

			/**
			 * @brief How many bindings are attached
			 */
			uint32_t initialized_bindings() const;

		private:
			DescriptorSetLayout const *_layout;
			uint32_t _cur_binding;
			uint32_t _frame_count;

			using Test = const VkWriteDescriptorSet;
			// Each contain one entry per descriptor binding
			std::vector<VkWriteDescriptorSet> _descriptor_writes;
			std::vector<std::vector<VkDescriptorBufferInfo>> _buffer_infos;
			std::vector<std::vector<VkDescriptorImageInfo>> _image_infos;

			std::vector<size_t> _buffer_ranges;

			std::vector<std::vector<VkImageView>> _image_views;
			std::vector<VkImageLayout> _image_layout;
	};

	/**
	 * @brief A collection of resources that can be used by a pipeline
	 */
	class DescriptorSets {
		public:
			/**
			 * @brief Creates an uninitialized DescriptorSet
			 */
			DescriptorSets() = default;
			/**
			 * @brief Creates a DescriptorSet with the information provided by the builder
			 */
			static util::Result<DescriptorSets, Error> create(
				DescriptorSetBuilder &builder,
				DescriptorPool const &pool
			);

			DescriptorSets(const DescriptorSets &other) = delete;
			DescriptorSets(DescriptorSets &&other);
			DescriptorSets &operator=(DescriptorSets const &other) = delete;
			DescriptorSets &operator=(DescriptorSets &&other);

			void destroy();
			~DescriptorSets();

			/**
			 * @brief Is the DescriptorSet initialized
			 */
			bool has_value() const;
			/**
			 * @brief Is the DescriptorSet initialized
			 */
			operator bool() const;

			/**
			 * @brief Gets the descriptor set for a specific frame
			 */
			const VkDescriptorSet descriptor_set(uint32_t frame_index = 0) const;

		private:
			std::vector<VkDescriptorSet> _descriptor_sets;
			DescriptorPool const *_descriptor_pool;
	};
}
