#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "resourceManager.h"
#include "vertex.h"
#include "vulkan/vulkan_core.h"
#include <vector>
#include "texture.h"

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
			VkImageView imageView() const;
			VkRenderPass renderPass() const;
			std::vector<VkBuffer> const &uniformBuffers() const;

		private:
			void createSyncObjects_();
			void createCommandBuffers_();
			void createRenderPass_();
			void createUniformBuffers_();
			void createDepthResources_();
			void createResultImages_();
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
			std::vector<VkFence> inFlightFences_;
			std::vector<VkSemaphore> renderFinishedSemaphores_;
			std::vector<VkCommandBuffer> commandBuffers_;

			VkImage depthImage_;
			VkDeviceMemory depthImageMemory_;
			VkImageView depthImageView_;
			VkExtent2D size_;
			const static VkFormat RESULT_IMAGE_FORMAT_ = VK_FORMAT_R8G8B8A8_SRGB;
			std::vector<VkImage> resultImages_;
			std::vector<VkImageView> resultImageViews_;
			std::vector<VkDeviceMemory> resultImageMemory_;
			std::vector<VkFramebuffer> resultImageFramebuffer_;
			std::vector<VkDescriptorSet> resultDescriptorSets_;
			int frameIndex_;

			uint32_t mipLevels_;
			bool framebufferResized_;

			types::ResourceManager const &resourceManager_;
	};
}
