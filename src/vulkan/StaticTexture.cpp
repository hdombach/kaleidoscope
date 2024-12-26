#include <filesystem>
#include <memory>

#include <vulkan/vulkan_core.h>
#include <stb_image.h>

#include "StaticTexture.hpp"
#include "graphics.hpp"
#include "imgui_impl_vulkan.h"

namespace vulkan {
	util::Result<StaticTexture::Ptr, KError> StaticTexture::from_file(
			uint32_t id,
			const std::string &url)
	{
		auto result = std::unique_ptr<StaticTexture>(new StaticTexture());
		result->_id = id;

		int tex_width, texHeight, texChannels;
		stbi_hdr_to_ldr_gamma(1.0f);
		auto pixels = stbi_load(
				url.c_str(),
				&tex_width,
				&texHeight,
				&texChannels,
				STBI_rgb_alpha);

		auto image_size = (VkDeviceSize) tex_width * texHeight * 4;

		result->_mip_levels = 1;

		if (!pixels) {
			return KError::invalid_image_file(url);
		}

		auto staging_buffer = VkBuffer{};
		auto staging_buffer_memory = VkDeviceMemory{};
		Graphics::DEFAULT->create_buffer(
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
	
		{
			auto size = VkExtent2D{
				static_cast<uint32_t>(tex_width),
					static_cast<uint32_t>(texHeight)
			};
			auto image = Image::create(
					size,
					VK_FORMAT_R8G8B8A8_UNORM,
					VK_IMAGE_USAGE_TRANSFER_SRC_BIT
					| VK_IMAGE_USAGE_TRANSFER_DST_BIT
					| VK_IMAGE_USAGE_SAMPLED_BIT,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			TRY(image);
			result->_image = std::move(image.value());

		}

		Graphics::DEFAULT->transition_image_layout(
				result->_image.image(),
				VK_FORMAT_R8G8B8A8_UNORM,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1);

		Graphics::DEFAULT->copy_buffer_to_image(
				staging_buffer,
				result->_image.image(),
				static_cast<uint32_t>(tex_width),
				static_cast<uint32_t>(texHeight));

		vkDestroyBuffer(Graphics::DEFAULT->device(), staging_buffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), staging_buffer_memory, nullptr);

		Graphics::DEFAULT->generate_mipmaps(result->_image.image(),
				VK_FORMAT_R8G8B8A8_UNORM,
				tex_width,
				texHeight,
				result->_mip_levels);

		result->_imgui_descriptor_set = ImGui_ImplVulkan_AddTexture(
				*Graphics::DEFAULT->main_texture_sampler(),
				result->_image.image_view(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		result->_name = std::filesystem::path(url).stem();

		return std::move(result);
	}

	StaticTexture::~StaticTexture() {
		_image.destroy();

		ImGui_ImplVulkan_RemoveTexture(_imgui_descriptor_set);
	}

	VkDescriptorSet StaticTexture::imgui_descriptor_set() {
		return _imgui_descriptor_set;
	}

	VkImageView StaticTexture::image_view() const {
		return _image.image_view();
	}

	uint32_t StaticTexture::id() const {
		return _id;
	}
}
