#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "image.hpp"
#include "imageView.hpp"
#include "../util/result.hpp"
#include "../util/errors.hpp"
#include "texture.hpp"
#include "../types/resourceManager.hpp"
#include "uniformBufferObject.hpp"

namespace vulkan {
	class PreviewRenderPassCore {
		public:
			static util::Result<PreviewRenderPassCore, KError> create(VkExtent2D size);

			PreviewRenderPassCore() = default;

			PreviewRenderPassCore(const PreviewRenderPassCore& other) = delete;
			PreviewRenderPassCore(PreviewRenderPassCore &&other);
			PreviewRenderPassCore& operator=(const PreviewRenderPassCore& other) = delete;
			PreviewRenderPassCore& operator=(PreviewRenderPassCore &&other);

			~PreviewRenderPassCore();

			void resize(VkExtent2D new_size);
			VkExtent2D size() const { return _size; }

			Image& depth_image() { return _depth_image; }
			Image const& depth_image() const { return _depth_image; }

			ImageView& depth_image_view() { return _depth_image_view; }
			ImageView const& depth_image_view() const { return _depth_image_view; }

			Image& color_image(int frame_index);
			Image const& color_image(int frame_index) const;

			ImageView& color_image_view(int frame_index);
			ImageView const& color_image_view(int frame_index) const;

			VkFramebuffer framebuffer(int frame_index);
			VkDescriptorSet imgui_descriptor_set(int frame_index);

			VkRenderPass render_pass();
		private:
			VkExtent2D _size;
			Image _depth_image;
			ImageView _depth_image_view;
			std::vector<Image> _color_images;
			std::vector<ImageView> _color_image_views;
			std::vector<VkFramebuffer> _framebuffers;
			std::vector<VkDescriptorSet> _imgui_descriptor_sets;

			VkRenderPass _render_pass;

			const static VkFormat _RESULT_IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;

			util::Result<void, KError> _create_images();
			void _cleanup_images();
			static VkFormat _depth_format();
	};

	class PreviewRenderPass: public Texture  {
		public:
			using Ptr = std::unique_ptr<PreviewRenderPass>;
			static util::Result<Ptr, KError> create(
					types::ResourceManager &resource_manager,
					VkExtent2D size);
			~PreviewRenderPass();
			void submit();
			void resize(glm::ivec2 size) override;
			bool is_resizable() const override;

			VkExtent2D size() const;
			VkDescriptorSet get_descriptor_set() override;
			ImageView const &image_view() override;
			VkRenderPass render_pass();
			std::vector<MappedUniformObject> const &uniform_buffers() const;

		private:
			PreviewRenderPass(types::ResourceManager &resource_manager, VkExtent2D size);

			util::Result<void, KError> _create_sync_objects();
			void _create_command_buffers();

			void _record_command_buffer(VkCommandBuffer commandBuffer);
			void _update_uniform_buffer(uint32_t currentImage);

			PreviewRenderPassCore _preview_render_pass;
			std::vector<MappedUniformObject> _mapped_uniforms;
			std::vector<Fence> _in_flight_fences;
			std::vector<Semaphore> _render_finished_semaphores;
			std::vector<VkCommandBuffer> _command_buffers;

			VkExtent2D _size;
			const static VkFormat _RESULT_IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
			int _frame_index;

			uint32_t _mip_levels;

			types::ResourceManager &_resource_manager;
	};
}
