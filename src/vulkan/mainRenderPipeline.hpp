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

			VkExtent2D getSize() const;
			VkDescriptorSet get_descriptor_set() const override;
			ImageView const &image_view() const override;
			VkRenderPass renderPass() const;
			std::vector<VkBuffer> const &uniformBuffers() const;

		private:
			MainRenderPipeline(types::ResourceManager &resourceManager, VkExtent2D size);

			util::Result<void, KError> createSyncObjects_();
			void createCommandBuffers_();
			void createRenderPass_();
			void createUniformBuffers_();
			util::Result<void, KError> _create_depth_resources();
			util::Result<void, KError> createResultImages_();
			void recreateResultImages_();
			void cleanupResultImages_();
			void cleanupDepthResources_();

			void recordCommandBuffer_(VkCommandBuffer commandBuffer);
			void updateUniformBuffer_(uint32_t currentImage);

			VkFormat findDepthFormat_();

			VkRenderPass renderPass_;
			std::vector<VkBuffer> uniformBuffers_;
			std::vector<VkDeviceMemory> uniformBuffersMemory_;
			std::vector<void*> uniformBuffersMapped_;
			std::vector<Fence> inFlightFences_;
			std::vector<Semaphore> renderFinishedSemaphores_;
			std::vector<VkCommandBuffer> commandBuffers_;

			Image _depth_image;
			ImageView _depth_image_view;
			VkExtent2D size_;
			const static VkFormat RESULT_IMAGE_FORMAT_ = VK_FORMAT_R8G8B8A8_SRGB;
			std::vector<Image> _result_images;
			std::vector<ImageView> _resultImageViews;
			std::vector<VkFramebuffer> resultImageFramebuffer_;
			std::vector<VkDescriptorSet> resultDescriptorSets_;
			int frameIndex_;

			uint32_t mipLevels_;
			bool framebufferResized_;

			types::ResourceManager const &resourceManager_;
	};
}
