#include <vulkan/vulkan_core.h>

#include "Sampler.hpp"
#include "graphics.hpp"

namespace vulkan {
	util::Result<Sampler, Error> Sampler::create_linear() {
		auto result = Sampler();

		auto sampler_info = _default_template();

		sampler_info.magFilter = VK_FILTER_LINEAR;
		sampler_info.minFilter = VK_FILTER_LINEAR;
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		auto err = vkCreateSampler(Graphics::DEFAULT->device(), &sampler_info, nullptr, &result._sampler);
		if (err != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create linear sampler", {err});
		}

		return result;
	}

	util::Result<Sampler, Error> Sampler::create_nearest() {
		auto result = Sampler();

		auto sampler_info = _default_template();

		sampler_info.magFilter = VK_FILTER_NEAREST;
		sampler_info.minFilter = VK_FILTER_NEAREST;
		sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

		auto err = vkCreateSampler(Graphics::DEFAULT->device(), &sampler_info, nullptr, &result._sampler);
		if (err != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Could not create nearest sampler", {err});
		}

		return result;
	}

	Sampler::Sampler(): _sampler(nullptr) {}

	Sampler::Sampler(Sampler &&other) {
		_sampler = other._sampler;
		other._sampler = nullptr;
	}

	Sampler& Sampler::operator=(Sampler &&other) {
		destroy();

		_sampler = other._sampler;
		other._sampler = nullptr;

		return *this;
	}

	void Sampler::destroy() {
		if (_sampler) {
			vkDestroySampler(Graphics::DEFAULT->device(), _sampler, nullptr);
			_sampler = nullptr;
		}
	}

	bool Sampler::exists() const { return _sampler; }

	VkSampler &Sampler::get() { return _sampler; }

	VkSampler const &Sampler::get() const { return _sampler; }

	VkSamplerCreateInfo Sampler::_default_template() {
		auto properties = VkPhysicalDeviceProperties{};
		vkGetPhysicalDeviceProperties(Graphics::DEFAULT->physical_device(), &properties);

		auto info = VkSamplerCreateInfo{};
		info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.anisotropyEnable = VK_TRUE;
		info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		info.unnormalizedCoordinates = VK_FALSE;
		info.compareEnable = VK_FALSE;
		info.compareOp = VK_COMPARE_OP_ALWAYS;
		info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		info.minLod = 0;
		info.maxLod = 0;
		info.mipLodBias = 0.0f;

		return info;
	}
}
