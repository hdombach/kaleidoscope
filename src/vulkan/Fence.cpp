#include <vulkan/vulkan_core.h>

#include "Fence.hpp"
#include "graphics.hpp"

namespace vulkan {
	util::Result<Fence, VkResult> Fence::create() {
		return create(Graphics::DEFAULT->device());
	}

	util::Result<Fence, VkResult> Fence::create(VkDevice device) {
		auto info = VkFenceCreateInfo{};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VkFence fence;
		auto res = vkCreateFence(device, &info, nullptr, &fence);
		if (res == VK_SUCCESS) {
			return Fence(fence);
		} else {
			return res;
		}
	}

	Fence::Fence() {
		_fence = nullptr;
	}

	Fence::Fence(Fence &&other) {
		_fence = other._fence;
		other._fence = nullptr;
	}
	Fence& Fence::operator=(Fence&& other) {
		destroy();
		_fence = other._fence;
		other._fence = nullptr;
		return *this;
	}

	void Fence::destroy() {
		if (_fence) {
			vkDestroyFence(Graphics::DEFAULT->device(), _fence, nullptr);
			_fence = nullptr;
		}
	}

	Fence::operator bool() const {
		return _fence;
	}

	VkFence& Fence::get() {
		return _fence;
	}

	VkFence const& Fence::get() const {
		return _fence;
	}

	VkResult Fence::wait() {
		return vkWaitForFences(
				Graphics::DEFAULT->device(),
				1,
				&_fence,
				VK_TRUE,
				UINT64_MAX);
	}

	VkResult Fence::reset() {
		return vkResetFences(
				Graphics::DEFAULT->device(),
				1,
				&_fence);
	}

	Fence::Fence(VkFence fence): _fence(fence) {}
}
