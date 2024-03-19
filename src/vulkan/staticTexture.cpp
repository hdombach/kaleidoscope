#include <cmath>

#include <vulkan/vulkan_core.h>
#include <stb_image.h>

#include "staticTexture.hpp"
#include "graphics.hpp"
#include "imgui_impl_vulkan.h"

namespace vulkan {
	util::Result<StaticTexture *, KError> StaticTexture::from_file(
			const std::string &url)
	{
		auto result = new StaticTexture();

		int tex_width, texHeight, texChannels;
		auto pixels = stbi_load(
				url.c_str(),
				&tex_width,
				&texHeight,
				&texChannels,
				STBI_rgb_alpha);

		auto image_size = (VkDeviceSize) tex_width * texHeight * 4;

		result->_mip_levels = static_cast<uint32_t>(
				std::floor(std::log2(std::max(tex_width, texHeight))));

		if (!pixels) {
			return KError::invalid_image_file(url);
		}

		auto staging_buffer = VkBuffer{};
		auto staging_buffer_memory = VkDeviceMemory{};
		Graphics::DEFAULT->createBuffer(
				image_size,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				staging_buffer,
				staging_buffer_memory);

		void *data;
		vkMapMemory(Graphics::DEFAULT->device(), staging_buffer_memory, 0, image_size, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(image_size));
		vkUnmapMemory(Graphics::DEFAULT->device(), staging_buffer_memory);
		stbi_image_free(pixels);
		
		auto texture_res = Image::create_full(
				tex_width, 
				texHeight, 
				result->_mip_levels, 
				VK_FORMAT_R8G8B8A8_SRGB, 
				VK_IMAGE_TILING_OPTIMAL, 
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
					VK_IMAGE_USAGE_TRANSFER_DST_BIT |
					VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		TRY(texture_res);
		result->_texture = std::move(texture_res.value());

		Graphics::DEFAULT->transitionImageLayout(
				result->_texture.value(),
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				result->_mip_levels);

		Graphics::DEFAULT->copyBufferToImage(
				staging_buffer,
				result->_texture.value(),
				static_cast<uint32_t>(tex_width),
				static_cast<uint32_t>(texHeight));

		vkDestroyBuffer(Graphics::DEFAULT->device(), staging_buffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), staging_buffer_memory, nullptr);

		Graphics::DEFAULT->generateMipmaps(result->_texture.value(),
				VK_FORMAT_R8G8B8A8_SRGB,
				tex_width,
				texHeight,
				result->_mip_levels);

		auto image_view_res = result->_texture.create_image_view_full(
				VK_FORMAT_R8G8B8A8_SRGB,
				VK_IMAGE_ASPECT_COLOR_BIT,
				result->_mip_levels);
		TRY(image_view_res);
		result->_texture_view = std::move(image_view_res.value());

		ImGui_ImplVulkan_AddTexture(
				Graphics::DEFAULT->mainTextureSampler(),
				result->_texture_view.value(),
				VK_IMAGE_LAYOUT_GENERAL);

		return result;
	}

	StaticTexture::~StaticTexture() {
		_texture.~Image();
		_texture_view.~ImageView();

		ImGui_ImplVulkan_RemoveTexture(_imgui_descriptor_set);
	}

	VkDescriptorSet StaticTexture::get_descriptor_set() const {
		return _imgui_descriptor_set;
	}

	ImageView const &StaticTexture::image_view() const {
		return _texture_view;
	}
}
