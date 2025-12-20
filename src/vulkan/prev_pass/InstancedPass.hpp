#pragma once

#include <memory>

#include "util/result.hpp"
#include "vulkan/DescriptorSet.hpp"
#include "vulkan/Fence.hpp"
#include "vulkan/Semaphore.hpp"
#include "vulkan/Image.hpp"
#include "vulkan/Error.hpp"

namespace vulkan {
	/**
	 * @brief Render pass for the instanced preview render pass.
	 */
	class InstancedPass {
		public:
			using Ptr = std::unique_ptr<InstancedPass>;

			/*
			 * @brief Create an uninitialized InstancedPass
			 */
			InstancedPass() = default;

			/**
			 * @brief Creates a new Instanced pass
			 * @param[in] size Initial size of the render view
			 */
			static util::Result<Ptr, Error> create(VkExtent2D size);

			/**
			 * @bief Copy constructor
			 */
			InstancedPass(const InstancedPass &other) = delete;
			/**
			 * @brief Move constructor
			 */
			InstancedPass(InstancedPass &&other);
			/**
			 * @brief Copy assignment
			 */
			InstancedPass& operator=(const InstancedPass &other) = delete;
			/**
			 * @brief Move assignment
			 */
			InstancedPass& operator=(InstancedPass &&other);

			/**
			 * @brief Deconstructs the InstancedPass
			 */
			void destroy();
			~InstancedPass();

			/**
			 * @brief Checks if InstancedPass is initialized
			 */
			bool has_value() const;
			/**
			 * @brief Alias for has_value
			 */
			operator bool() const;

			/**
			 * @brief Submits draw commands to the graphics card
			 * @param[in] semaphore The semaphore to wait on before starting
			 * @returns The semaphore marking when the InstancedPass is done
			 */
			VkSemaphore render(VkSemaphore semaphore);

			/**
			 * @brief The render pass for the rasterization stage
			 * TODO: might not need to be used
			 */
			VkRenderPass render_pass();

			/**
			 * @brief Handle for the preview in imgui
			 */
			VkDescriptorSet imgui_descriptor_set();

			/**
			 * @brief The primary viewport
			 */
			VkImageView image_view();

		private:
			VkRenderPass _render_pass;
			VkPipeline _pipeline;
			VkPipelineLayout _pipeline_layout;
			DescriptorPool _descriptor_pool;
			DescriptorSets _descriptor_set;
			DescriptorSetLayout _descriptor_set_layout;
			Fence _fence;
			Semaphore _semaphore;
			VkCommandBuffer _command_buffer;
			VkFramebuffer _framebuffer;
			VkExtent2D _size;
			Image _depth_image;
			Image _material_image;
			VkDescriptorSet _imgui_descriptor_set;
			
		private:
			const static VkFormat _MATERIAL_IMAGE_FORMAT = VK_FORMAT_R16_UINT;

			/**
			 * @brief Sets up the render pass
			 * Does not depend on any other components to be initialized
			 */
			util::Result<VkRenderPass, Error> _create_render_pass();
			void _destroy_render_pass();

			/**
			 * @brief Sets up the pipeline
			 * Requires the descriptor set and render pass to be initialized.
			 */
			util::Result<void, Error>_create_pipeline(
				VkPipeline &pipeline,
				VkPipelineLayout &pipeline_layout
			);
			void _destroy_pipeline();

			/**
			 * @brief Creates the depth and material images
			 */
			util::Result<void, Error> _create_images();

			/**
			 * @brief Creates the command buffer
			 * Does not depend on any other components to be initialized.
			 */
			util::Result<VkCommandBuffer, Error> _create_command_buffer();
			void _destroy_command_buffer();

			/**
			 * @brief Creates the framebuffer
			 * Requires the images and render pass to be initialized.
			 */
			util::Result<VkFramebuffer, Error> _create_framebuffers();
			void _destroy_framebuffers();

			/**
			 * @brief Figures out the best supported depth format
			 * Does not depend on any other components to be initialized.
			 */
			VkFormat _depth_format();

			/**
			 * @brief Creates the descriptor set
			 * Depends on the images to be initialized.
			 */
			util::Result<void, Error> _create_descriptor_set();
	};
}
