#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "../types/resourceManager.hpp"
#include "semaphore.hpp"
#include "fence.hpp"
#include "vulkan/vulkan_core.h"
#include "texture.hpp"
#include "imageView.hpp"

namespace vulkan {
	class Graphics;

	class MainRenderPipeline: public Texture  {
		public:
			MainRenderPipeline(types::ResourceManager &resourceManager, VkExtent2D size);
			~MainRenderPipeline();
			void submit();
			void resize(glm::ivec2 size);
			bool isResizable() const;

			VkExtent2D getSize() const;
			VkDescriptorSet getDescriptorSet() const;
			ImageView const &imageView() const;
			VkRenderPass renderPass() const;
			std::vector<VkBuffer> const &uniformBuffers() const;

		private:
			util::Result<void, KError> createSyncObjects_();
			void createCommandBuffers_();
			void createRenderPass_();
			void createUniformBuffers_();
			void createDepthResources_();
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

			VkImage depthImage_;
			VkDeviceMemory depthImageMemory_;
			ImageView _depth_image_view;
			VkExtent2D size_;
			const static VkFormat RESULT_IMAGE_FORMAT_ = VK_FORMAT_R8G8B8A8_SRGB;
			std::vector<VkImage> resultImages_;
			std::vector<ImageView> _resultImageViews;
			std::vector<VkDeviceMemory> resultImageMemory_;
			std::vector<VkFramebuffer> resultImageFramebuffer_;
			std::vector<VkDescriptorSet> resultDescriptorSets_;
			int frameIndex_;

			uint32_t mipLevels_;
			bool framebufferResized_;

			types::ResourceManager const &resourceManager_;
	};
}
