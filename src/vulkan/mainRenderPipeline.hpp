#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../types/resourceManager.hpp"
#include "semaphore.hpp"
#include "fence.hpp"
#include "texture.hpp"
#include "imageView.hpp"
#include "image.hpp"
#include "../util/result.hpp"

namespace vulkan {
	class Graphics;

	class MainRenderPipeline: public Texture  {
		public:
			using Ptr = std::unique_ptr<MainRenderPipeline>;
			static util::Result<Ptr, KError> create(
					types::ResourceManager &resource_manager,
					VkExtent2D size);
			~MainRenderPipeline();
			void submit();
			void resize(glm::ivec2 size) override;
			bool is_resizable() const override;

			VkExtent2D get_size() const;
			VkDescriptorSet get_descriptor_set() const override;
			ImageView const &image_view() const override;
			VkRenderPass render_pass() const;
			std::vector<VkBuffer> const &uniform_buffers() const;

		private:
			MainRenderPipeline(types::ResourceManager &resource_manager, VkExtent2D size);

			util::Result<void, KError> _create_sync_objects();
			void _create_command_buffers();
			void _create_render_pass();
			void _create_uniform_buffers();
			util::Result<void, KError> _create_depth_resources();
			util::Result<void, KError> _create_result_images();
			void _recreate_result_images();
			void _cleanup_result_images();
			void _cleanup_depth_resources();

			void _record_command_buffer(VkCommandBuffer commandBuffer);
			void _update_uniform_buffer(uint32_t currentImage);

			VkFormat _find_depth_format();

			VkRenderPass _render_pass;
			std::vector<VkBuffer> _uniform_buffers;
			std::vector<VkDeviceMemory> _uniform_buffers_memory;
			std::vector<void*> _uniform_buffers_mapped;
			std::vector<Fence> _in_flight_fences;
			std::vector<Semaphore> _render_finished_semaphores;
			std::vector<VkCommandBuffer> _command_buffers;

			Image _depth_image;
			ImageView _depth_image_view;
			VkExtent2D _size;
			const static VkFormat _RESULT_IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
			std::vector<Image> _result_images;
			std::vector<ImageView> _result_image_views;
			std::vector<VkFramebuffer> _result_image_framebuffer;
			std::vector<VkDescriptorSet> _result_descriptor_sets;
			int _frame_index;

			uint32_t _mip_levels;
			bool _framebuffer_resized;

			types::ResourceManager const &_resource_manager;
	};
}
