#pragma once

#include <cstdint>

#include <vector>
#include <vulkan/vulkan_core.h>

#include "MappedUniform.hpp"
#include "DescriptorPool.hpp"
#include "StaticBuffer.hpp"

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

			static DescriptorSetTemplate create_uniform(
					uint32_t binding,
					VkShaderStageFlags stage_flags,
					Uniform &uniform)
			{
				auto buffers = std::vector<VkBuffer>{uniform.buffer()};
				return _create_uniform_impl(
						binding,
						stage_flags,
						std::move(buffers),
						uniform.size());
			}

			static DescriptorSetTemplate create_image(
					uint32_t binding,
					VkShaderStageFlags stage_flags,
					VkImageView image_view);

			static DescriptorSetTemplate create_image(
					uint32_t binding,
					VkShaderStageFlags stage_flags,
					VkImageView image_view,
					VkImageLayout image_layout);

			static util::Result<DescriptorSetTemplate, KError> create_images(
					uint32_t binding,
					VkShaderStageFlags stage_flags,
					std::vector<VkImageView> const &image_views);

			static DescriptorSetTemplate create_image_target(
					uint32_t binding,
					VkShaderStageFlags stage_flags,
					VkImageView image_view);

			static util::Result<DescriptorSetTemplate, KError> create_storage_buffer(
					uint32_t binding,
					VkShaderStageFlags stage_flags,
					VkBuffer buffer,
					size_t range);

			static util::Result<DescriptorSetTemplate, KError> create_storage_buffer(
					uint32_t binding,
					VkShaderStageFlags stage_flags,
					StaticBuffer &static_buffer);

			VkDescriptorSetLayoutBinding layout_binding() const { return _layout_binding; }
			std::vector<VkBuffer> const &buffers() const { return _buffers; }
			unsigned long buffer_range() const { return _buffer_range; }

			std::vector<VkImageView> const &image_views() const { return _image_views; }
			VkImageLayout image_layout() const { return _image_layout; }

		private:
			static DescriptorSetTemplate _create_uniform_impl(
					uint32_t binding,
					VkShaderStageFlags stage_flags,
					std::vector<VkBuffer> &&buffers,
					size_t buffer_size);

		private:
			VkDescriptorSetLayoutBinding _layout_binding;

			// used for buffer descirptor set
			std::vector<VkBuffer> _buffers;
			unsigned long _buffer_range;

			// Used for image descriptor set
			std::vector<VkImageView> _image_views;
			VkImageLayout _image_layout;
	};

	class DescriptorSetLayout {
		public:
			static util::Result<DescriptorSetLayout, KError> create(
					std::vector<DescriptorSetTemplate> &templates);
			static util::Result<DescriptorSetLayout, KError> create(
					VkDescriptorSetLayoutCreateInfo &layout_info);

			DescriptorSetLayout(const DescriptorSetLayout& other) = delete;
			DescriptorSetLayout(DescriptorSetLayout &&other);
			DescriptorSetLayout& operator=(const DescriptorSetLayout& other) = delete;
			DescriptorSetLayout& operator=(DescriptorSetLayout&& other);
			DescriptorSetLayout();

			~DescriptorSetLayout() { destroy(); }
			void destroy();

			VkDescriptorSetLayout layout() { return _descriptor_set_layout; }
			VkDescriptorSetLayout *layout_ptr() { return &_descriptor_set_layout; }

		private:
			VkDescriptorSetLayout _descriptor_set_layout;
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
			void destroy();

			VkDescriptorSet descriptor_set(uint32_t frame_index);
			VkDescriptorSetLayout layout();
			VkDescriptorSetLayout *layout_ptr();
			DescriptorPool &descriptor_pool();
			bool is_cleared();
		private:
			std::vector<VkDescriptorSet> _descriptor_sets;
			DescriptorSetLayout _descriptor_set_layout;
			DescriptorPool *_descriptor_pool;
	};
}
