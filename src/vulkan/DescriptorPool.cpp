#include "DescriptorPool.hpp"
#include "error.hpp"
#include "graphics.hpp"
#include "vulkan/vulkan_core.h"
#include <array>

namespace vulkan {
	DescriptorPool DescriptorPool::create() {
		auto result = DescriptorPool();

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
					&result._descriptor_pool));

		return result;
	}

	DescriptorPool::DescriptorPool():
		_descriptor_pool(nullptr)
	{ }

	DescriptorPool::DescriptorPool(DescriptorPool &&other) {
		_descriptor_pool = other._descriptor_pool;
		other._descriptor_pool = nullptr;
	}

	DescriptorPool& DescriptorPool::operator=(DescriptorPool&& other) {
		this->~DescriptorPool();

		_descriptor_pool = other._descriptor_pool;
		other._descriptor_pool = nullptr;

		return *this;
	}

	DescriptorPool::~DescriptorPool() {
		if (_descriptor_pool) {
			vkDestroyDescriptorPool(
					Graphics::DEFAULT->device(),
					_descriptor_pool,
					nullptr);
			_descriptor_pool = nullptr;
		}
	}

	VkDescriptorPool const &DescriptorPool::descriptor_pool() const {
		return _descriptor_pool;
	}
}
