#pragma once

#include <cstdint>
#include <limits>
#include <variant>

#include <vulkan/vulkan_core.h>

#include "util/result.hpp"
#include "util/KError.hpp"
#include "DescriptorPool.hpp"
#include "MappedUniform.hpp"
#include "StaticBuffer.hpp"

namespace vulkan {
	static const uint32_t DESCRIPTOR_BINDING_UNUSED = std::numeric_limits<uint32_t>::max();

	VkDescriptorSetLayoutBinding descriptor_layout_uniform(
		VkShaderStageFlags stage_flags
	);

	VkDescriptorSetLayoutBinding descriptor_layout_image(VkShaderStageFlags stage_flags);

	VkDescriptorSetLayoutBinding descriptor_layout_images(
		VkShaderStageFlags stage_flags,
		size_t image_count
	);

	VkDescriptorSetLayoutBinding descriptor_layout_image_target(
		VkShaderStageFlags stage_flags,
		size_t image_count
	);
	VkDescriptorSetLayoutBinding descriptor_layout_storage_buffer(
		VkShaderStageFlags stage_flags
	);

	class DescriptorSetBuilder;

	class DescriptorSetLayout {
		public:
			DescriptorSetLayout() = default;

			static util::Result<DescriptorSetLayout, KError> create(
				std::vector<VkDescriptorSetLayoutBinding> const &bindings
			);

			DescriptorSetLayout(DescriptorSetLayout const &other) = delete;
			DescriptorSetLayout(DescriptorSetLayout &&other);
			DescriptorSetLayout& operator=(DescriptorSetLayout const &other) = delete;
			DescriptorSetLayout& operator=(DescriptorSetLayout &&other);

			void destroy();
			~DescriptorSetLayout();

			bool has_value() const;
			operator bool() const;

			DescriptorSetBuilder builder(uint32_t frame_count = 1);

			VkDescriptorSetLayout &layout();

			std::vector<VkDescriptorSetLayoutBinding> const &bindings() const;

		private:
			VkDescriptorSetLayout _layout = nullptr;
			std::vector<VkDescriptorSetLayoutBinding> _bindings;
	};

	class DescriptorSetBuilder {
		public:
			DescriptorSetBuilder() = default;
			DescriptorSetBuilder(DescriptorSetLayout &layout, uint32_t frame_count);

			//Internal points need to remain valid
			DescriptorSetBuilder(DescriptorSetBuilder const &other) = delete;
			DescriptorSetBuilder(DescriptorSetBuilder &&other);
			DescriptorSetBuilder &operator=(DescriptorSetBuilder const &other) = delete;
			DescriptorSetBuilder &operator=(DescriptorSetBuilder &&other);

			bool has_value() const;
			operator bool() const;

			DescriptorSetLayout &layout();
			VkDescriptorSetLayout *vk_layout();

			std::vector<VkWriteDescriptorSet> &writes();

			uint32_t frame_count() const;

			template<typename BufferObj>
			util::Result<DescriptorSetBuilder &, KError> add_uniform(
				std::vector<MappedUniform<BufferObj>> &mapped_uniforms
			) {
				auto buffers = std::vector<VkBuffer>();
				for (auto &uniform : mapped_uniforms) {
					buffers.push_back(uniform.buffer());
				}

				return add_uniform(std::move(buffers), sizeof(BufferObj));
			}

			template<typename BufferObj>
			util::Result<DescriptorSetBuilder &, KError> add_uniform(
				MappedUniform<BufferObj> &mapped_uniform
			) {
				return add_uniform({mapped_uniform});
			}

			util::Result<DescriptorSetBuilder &, KError> add_uniform(
				Uniform &uniform
			);

			util::Result<DescriptorSetBuilder &, KError> add_uniform(
				std::vector<VkBuffer> const &buffers,
				size_t buffer_size
			);

			util::Result<DescriptorSetBuilder &, KError> add_image(
				VkImageView image_view
			);

			util::Result<DescriptorSetBuilder &, KError> add_image(
				VkImageView image_view,
				VkImageLayout image_layout
			);

			util::Result<DescriptorSetBuilder &, KError> add_image(
				std::vector<VkImageView> const &image_views
			);

			util::Result<DescriptorSetBuilder &, KError> add_image_target(
				VkImageView image_view
			);

			util::Result<DescriptorSetBuilder &, KError> add_storage_buffer(
				VkBuffer buffer,
				size_t range
			);

			util::Result<DescriptorSetBuilder &, KError> add_storage_buffer(
				StaticBuffer &static_buffer
			);

			DescriptorSetBuilder &set_sampler(uint32_t binding, Sampler const &sampler);

			uint32_t initialized_bindings() const;

		private:
			DescriptorSetLayout *_layout;
			uint32_t _cur_binding;
			uint32_t _frame_count;

			// Each contain one entry per descriptor binding
			std::vector<VkWriteDescriptorSet> _descriptor_writes;
			std::vector<std::vector<VkDescriptorBufferInfo>> _buffer_infos;
			std::vector<std::vector<VkDescriptorImageInfo>> _image_infos;

			std::vector<size_t> _buffer_ranges;

			std::vector<std::vector<VkImageView>> _image_views;
			std::vector<VkImageLayout> _image_layout;
	};

	class DescriptorSets {
		public:
			DescriptorSets() = default;
			static util::Result<DescriptorSets, KError> create(
				DescriptorSetBuilder &builder,
				DescriptorPool &pool
			);

			DescriptorSets(const DescriptorSets &other) = delete;
			DescriptorSets(DescriptorSets &&other);
			DescriptorSets &operator=(DescriptorSets const &other) = delete;
			DescriptorSets &operator=(DescriptorSets &&other);

			void destroy();
			~DescriptorSets();

			bool has_value() const;
			operator bool() const;

			VkDescriptorSet descriptor_set(uint32_t frame_index = 0);

		private:
			std::vector<VkDescriptorSet> _descriptor_sets;
			DescriptorPool *_descriptor_pool;
	};
}
