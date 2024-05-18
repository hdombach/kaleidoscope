#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "DescriptorPool.hpp"
#include "DescriptorSet.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include "../util/result.hpp"
#include "../util/errors.hpp"
#include "Texture.hpp"
#include "../types/ResourceManager.hpp"
#include "UniformBufferObject.hpp"

namespace vulkan {
	class PreviewRenderPass: public Texture  {
		public:
			using Ptr = std::unique_ptr<PreviewRenderPass>;
			static util::Result<Ptr, KError> create(
					types::ResourceManager &resource_manager,
					VkExtent2D size);
			~PreviewRenderPass();
			void submit(std::function<void(VkCommandBuffer)> render_callback);
			void resize(VkExtent2D size) override;
			bool is_resizable() const override;

			VkExtent2D size() const;
			VkDescriptorSet get_descriptor_set() override;
			ImageView const &image_view() override;
			VkRenderPass render_pass();
			MappedGlobalUniform &current_uniform_buffer();
			DescriptorPool &descriptor_pool() { return _descriptor_pool; };
			int frame_index() { return _frame_index; };
			VkDescriptorSetLayout global_descriptor_set_layout() { return _descriptor_sets.layout(); }
			VkDescriptorSet global_descriptor_set(int frame_index) { return _descriptor_sets.descriptor_set(frame_index); }

		private:
			PreviewRenderPass(types::ResourceManager &resource_manager, VkExtent2D size);

			util::Result<void, KError> _create_sync_objects();
			void _create_command_buffers();
			util::Result<void, KError> _create_images();
			void _cleanup_images();
			static VkFormat _depth_format();

			VkExtent2D _size;
			Image _depth_image;
			ImageView _depth_image_view;
			std::vector<Image> _color_images;
			std::vector<ImageView> _color_image_views;
			std::vector<VkFramebuffer> _framebuffers;
			std::vector<VkDescriptorSet> _imgui_descriptor_sets;
			DescriptorSets _descriptor_sets;
			VkRenderPass _render_pass;
			std::vector<MappedGlobalUniform> _mapped_uniforms;
			std::vector<Fence> _in_flight_fences;
			std::vector<Semaphore> _render_finished_semaphores;
			std::vector<VkCommandBuffer> _command_buffers;
			DescriptorPool _descriptor_pool;
			const static VkFormat _RESULT_IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
			int _frame_index;
			uint32_t _mip_levels;

			types::ResourceManager &_resource_manager;
	};
}
