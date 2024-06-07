#pragma once

#include <cstdint>

#include <vector>
#include <vulkan/vulkan_core.h>

#include "MappedUniform.hpp"
#include "DescriptorPool.hpp"

namespace vulkan {
	class DescriptorSetTemplate {
		public:

			template<typename BufferObj>
			static DescriptorSetTemplate create_uniform(
					uint32_t binding,
					VkShaderStageFlags stage_flags, 
					std::vector<MappedUniform<BufferObj>> &mapped_uniform)
			{
				auto buffers = std::vector<VkBuffer>();
				for (auto &uniform : mapped_uniform) {
					buffers.push_back(uniform.buffer());
				}
				return _create_uniform_impl(
						binding, 
						stage_flags, 
						std::move(buffers), 
						sizeof(BufferObj));
			}

			template<typename BufferObj>
			static DescriptorSetTemplate create_uniform(
					uint32_t binding,
					VkShaderStageFlags stage_flags,
					MappedUniform<BufferObj> &mapped_uniform)
			{
				auto buffers = std::vector<VkBuffer>();
				buffers.push_back(mapped_uniform.buffer());
				return _create_uniform_impl(
						binding, 
						stage_flags, 
						std::move(buffers), 
						sizeof(BufferObj));
			}

			static DescriptorSetTemplate create_image(
					uint32_t binding,
					VkShaderStageFlags stage_flags,
					ImageView const &image_view);

			static DescriptorSetTemplate create_image_target(
					uint32_t binding,
					VkShaderStageFlags stage_flags,
					ImageView const &image_view);

			static DescriptorSetTemplate create_storage_buffer(
					uint32_t binding,
					VkShaderStageFlags stage_flags,
					VkBuffer buffer,
					size_t range);

			VkDescriptorSetLayoutBinding layout_binding() const { return _layout_binding; }
			std::vector<VkBuffer> const &buffers() const { return _buffers; }
			unsigned long buffer_range() const { return _buffer_range; }

			VkImageView const image_view() const { return _image_view; }

		private:
			VkDescriptorSetLayoutBinding _layout_binding;

			// used for buffer descirptor set
			std::vector<VkBuffer> _buffers;
			unsigned long _buffer_range;

			// Used for image descriptor set
			VkImageView _image_view;

			static DescriptorSetTemplate _create_uniform_impl(
					uint32_t binding,
					VkShaderStageFlags stage_flags,
					std::vector<VkBuffer> &&buffers,
					size_t buffer_size);
	};

	class DescriptorSets {
		public:
			static util::Result<DescriptorSets, KError> create(
					std::vector<DescriptorSetTemplate> &templates,
					uint32_t frame_count,
					DescriptorPool &descriptor_pool);

			DescriptorSets(const DescriptorSets& other) = delete;
			DescriptorSets(DescriptorSets &&other);
			DescriptorSets& operator=(const DescriptorSets& other) = delete;
			DescriptorSets& operator=(DescriptorSets&& other);
			DescriptorSets();

			~DescriptorSets();
			void clear();

			VkDescriptorSet descriptor_set(uint32_t frame_index);
			VkDescriptorSetLayout layout();
			VkDescriptorSetLayout *layout_ptr();
			DescriptorPool &descriptor_pool();
			bool is_cleared();
		private:
			std::vector<VkDescriptorSet> _descriptor_sets;
			VkDescriptorSetLayout _descriptor_set_layout;
			DescriptorPool *_descriptor_pool;
	};
}
