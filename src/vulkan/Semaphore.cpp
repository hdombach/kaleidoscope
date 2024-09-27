#include <vulkan/vulkan_core.h>

#include "Semaphore.hpp"
#include "graphics.hpp"

namespace vulkan {
	util::Result<Semaphore, VkResult> Semaphore::create() {
		return create(Graphics::DEFAULT->device());
	}

	util::Result<Semaphore, VkResult> Semaphore::create(VkDevice device) {
		auto info = VkSemaphoreCreateInfo{};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkSemaphore semaphore;
		auto res = vkCreateSemaphore(
				device,
				&info,
				nullptr,
				&semaphore);
		if (res == VK_SUCCESS) {
			return Semaphore(semaphore);
		} else {
			return {res};
		}
	}

	Semaphore::Semaphore(): _semaphore(nullptr) {}

	Semaphore::Semaphore(Semaphore &&other) {
		_semaphore = other._semaphore;
		other._semaphore = nullptr;
	}
	Semaphore& Semaphore::operator=(Semaphore&& other) {
		_semaphore = other._semaphore;
		other._semaphore = nullptr;
		return *this;
	}

	void Semaphore::destroy() {
		if (_semaphore) {
			vkDestroySemaphore(Graphics::DEFAULT->device(), _semaphore, nullptr);
			_semaphore = nullptr;
		}
	}

	Semaphore::operator bool() const {
		return _semaphore;
	}

	VkSemaphore& Semaphore::get() {
		return _semaphore;
	}

	VkSemaphore const& Semaphore::get() const {
		return _semaphore;
	}

	Semaphore::Semaphore(VkSemaphore semaphore): _semaphore(semaphore) {}
}
