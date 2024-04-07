#include "descriptorPool.hpp"
#include "error.hpp"
#include "graphics.hpp"
#include "vulkan/vulkan_core.h"
#include <array>

namespace vulkan {
	DescriptorPool::DescriptorPool() {
		auto poolSizes = std::array<VkDescriptorPoolSize, 3>{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 100;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = 100;
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[2].descriptorCount = 100;

		auto poolInfo = VkDescriptorPoolCreateInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 100;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		require(vkCreateDescriptorPool(
					Graphics::DEFAULT->device(),
					&poolInfo,
					nullptr,
					&descriptorPool_));
	}

	DescriptorPool::~DescriptorPool() {
		vkDestroyDescriptorPool(
				Graphics::DEFAULT->device(),
				descriptorPool_,
				nullptr);
	}

	VkDescriptorPool const &DescriptorPool::descriptorPool() const {
		return descriptorPool_;
	}
}
