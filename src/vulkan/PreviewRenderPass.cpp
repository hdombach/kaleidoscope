#include <vulkan/vulkan_core.h>

#include "PreviewRenderPass.hpp"
#include "graphics.hpp"

namespace vulkan {
	util::Result<PreviewRenderPass, KError> PreviewRenderPass::create(
			VkExtent2D size)
	{
		auto result = PreviewRenderPass();
		result._size = size;
		TRY(result._create_images());
		return result;
	}

	void PreviewRenderPass::resize(VkExtent2D new_size) {
		Graphics::DEFAULT->waitIdle();
		_cleanup_images();
		_size = new_size;
		_create_images();
	}

	util::Result<void, KError> PreviewRenderPass::_create_images() {
		/* create depth resource */
		{
			auto image_res = Image::create(
					_size.width,
					_size.height,
					_depth_format(),
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
			TRY(image_res);
			_depth_image = std::move(image_res.value());

			auto image_view_res = _depth_image.create_image_view_full(
					_depth_format(), 
					VK_IMAGE_ASPECT_DEPTH_BIT, 
					1);
			TRY(image_view_res);
			_depth_image_view = std::move(image_view_res.value());

			Graphics::DEFAULT->transitionImageLayout(
					_depth_image.value(), 
					_depth_format(), 
					VK_IMAGE_LAYOUT_UNDEFINED, 
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 
					1);

		}

		return {};
	}

	void PreviewRenderPass::_cleanup_images() {
		_depth_image.~Image();
		_depth_image_view.~ImageView();
	}

	VkFormat PreviewRenderPass::_depth_format() {
		return Graphics::DEFAULT->findSupportedFormat(
				{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, 
				VK_IMAGE_TILING_OPTIMAL, 
				VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
}
