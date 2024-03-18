#include <cmath>

#include <vulkan/vulkan_core.h>
#include <stb_image.h>

#include "staticTexture.hpp"
#include "graphics.hpp"
#include "imgui_impl_vulkan.h"

namespace vulkan {
	util::Result<StaticTexture *, KError> StaticTexture::fromFile(
			const std::string &url)
	{
		auto result = new StaticTexture();

		int texWidth, texHeight, texChannels;
		auto pixels = stbi_load(
				url.c_str(),
				&texWidth,
				&texHeight,
				&texChannels,
				STBI_rgb_alpha);

		auto imageSize = (VkDeviceSize) texWidth * texHeight * 4;

		result->_mipLevels = static_cast<uint32_t>(
				std::floor(std::log2(std::max(texWidth, texHeight))));

		if (!pixels) {
			return KError::invalid_image_file(url);
		}

		auto stagingBuffer = VkBuffer{};
		auto stagingBufferMemory = VkDeviceMemory{};
		Graphics::DEFAULT->createBuffer(
				imageSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer,
				stagingBufferMemory);

		void *data;
		vkMapMemory(Graphics::DEFAULT->device(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(Graphics::DEFAULT->device(), stagingBufferMemory);
		stbi_image_free(pixels);

		Graphics::DEFAULT->createImage(
				texWidth,
				texHeight,
				result->_mipLevels,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
					VK_IMAGE_USAGE_TRANSFER_DST_BIT |
					VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				result->_texture,
				result->_textureMemory);

		Graphics::DEFAULT->transitionImageLayout(
				result->_texture,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				result->_mipLevels);

		Graphics::DEFAULT->copyBufferToImage(
				stagingBuffer,
				result->_texture,
				static_cast<uint32_t>(texWidth),
				static_cast<uint32_t>(texHeight));

		vkDestroyBuffer(Graphics::DEFAULT->device(), stagingBuffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), stagingBufferMemory, nullptr);

		Graphics::DEFAULT->generateMipmaps(result->_texture,
				VK_FORMAT_R8G8B8A8_SRGB,
				texWidth,
				texHeight,
				result->_mipLevels);

		auto image_view_info = ImageView::create_info(result->_texture);
		image_view_info.subresourceRange.levelCount = result->_mipLevels;
		auto image_view = ImageView::create(image_view_info);
		TRY(image_view);
		result->_textureView = std::move(image_view.value());

		ImGui_ImplVulkan_AddTexture(
				Graphics::DEFAULT->mainTextureSampler(),
				result->_textureView.value(),
				VK_IMAGE_LAYOUT_GENERAL);

		return result;
	}

	StaticTexture::~StaticTexture() {
		vkDestroyImage(Graphics::DEFAULT->device(), _texture, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), _textureMemory, nullptr);
		_textureView.~ImageView();

		ImGui_ImplVulkan_RemoveTexture(_imguiDescriptorSet);
	}

	VkDescriptorSet StaticTexture::getDescriptorSet() const {
		return _imguiDescriptorSet;
	}

	ImageView const &StaticTexture::imageView() const {
		return _textureView;
	}
}
