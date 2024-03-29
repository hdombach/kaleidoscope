#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../types/resourceManager.hpp"
#include "PreviewRenderPass.hpp"
#include "uniformBufferObject.hpp"
#include "semaphore.hpp"
#include "fence.hpp"
#include "texture.hpp"
#include "imageView.hpp"
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
			VkDescriptorSet get_descriptor_set() override;
			ImageView const &image_view() override;
			VkRenderPass render_pass();
			std::vector<MappedUniformObject> const &uniform_buffers() const;

		private:
			MainRenderPipeline(types::ResourceManager &resource_manager, VkExtent2D size);

			util::Result<void, KError> _create_sync_objects();
			void _create_command_buffers();
			void _recreate_result_images();

			void _record_command_buffer(VkCommandBuffer commandBuffer);
			void _update_uniform_buffer(uint32_t currentImage);

			VkFormat _find_depth_format();

			PreviewRenderPass _preview_render_pass;
			std::vector<MappedUniformObject> _mapped_uniforms;
			std::vector<Fence> _in_flight_fences;
			std::vector<Semaphore> _render_finished_semaphores;
			std::vector<VkCommandBuffer> _command_buffers;

			VkExtent2D _size;
			const static VkFormat _RESULT_IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
			int _frame_index;

			uint32_t _mip_levels;
			bool _framebuffer_resized;

			types::ResourceManager &_resource_manager;
	};
}
