#include "staticTexture.h"
#include "graphics.h"
#include "imgui_impl_vulkan.h"
#include "vulkan/vulkan_core.h"
#include <cmath>
#include <stb_image.h>
#include <stdexcept>

namespace vulkan {
	util::Result<StaticTexture *, errors::InvalidImageFile> StaticTexture::fromFile(
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

		result->mipLevels_ = static_cast<uint32_t>(
				std::floor(std::log2(std::max(texWidth, texHeight))));

		if (!pixels) {
			return errors::InvalidImageFile{url};
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
				result->mipLevels_,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
					VK_IMAGE_USAGE_TRANSFER_DST_BIT |
					VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				result->texture_,
				result->textureMemory_);

		Graphics::DEFAULT->transitionImageLayout(
				result->texture_,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				result->mipLevels_);

		Graphics::DEFAULT->copyBufferToImage(
				stagingBuffer,
				result->texture_,
				static_cast<uint32_t>(texWidth),
				static_cast<uint32_t>(texHeight));

		vkDestroyBuffer(Graphics::DEFAULT->device(), stagingBuffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), stagingBufferMemory, nullptr);

		Graphics::DEFAULT->generateMipmaps(result->texture_,
				VK_FORMAT_R8G8B8A8_SRGB,
				texWidth,
				texHeight,
				result->mipLevels_);

		result->textureView_ = Graphics::DEFAULT->createImageView(
				result->texture_,
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_ASPECT_COLOR_BIT,
				result->mipLevels_);

		ImGui_ImplVulkan_AddTexture(
				Graphics::DEFAULT->mainTextureSampler(),
				result->textureView_,
				VK_IMAGE_LAYOUT_GENERAL);

		return result;
	}

	StaticTexture::~StaticTexture() {
		vkDestroyImage(Graphics::DEFAULT->device(), texture_, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), textureMemory_, nullptr);
		vkDestroyImageView(Graphics::DEFAULT->device(), textureView_, nullptr);

		ImGui_ImplVulkan_RemoveTexture(imguiDescriptorSet_);
	}

	VkDescriptorSet StaticTexture::getDescriptorSet() const {
		return imguiDescriptorSet_;
	}

	VkImageView StaticTexture::imageView() const {
		return textureView_;
	}
}
