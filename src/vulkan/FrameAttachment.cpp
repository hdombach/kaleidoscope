#include "FrameAttachment.hpp"

#include <vulkan/vulkan_core.h>

#include "Image.hpp"
#include "vulkan/Error.hpp"

namespace vulkan {
	FrameAttachment FrameAttachment::create(const Image &image) {
		auto attachment = FrameAttachment();

		attachment._image = &image;

		switch (image.format()) {
			case VK_FORMAT_D32_SFLOAT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
			case VK_FORMAT_D24_UNORM_S8_UINT:
				attachment._clear_color = {1.0f, 0};
				break;
			case VK_FORMAT_R16_UINT:
				attachment._clear_color = {0};
				break;
			case VK_FORMAT_R8_SRGB:
				attachment._clear_color = {0};
				break;
			case VK_FORMAT_R32G32_SFLOAT:
				attachment._clear_color = {0.0, 0.0};
				break;
			case VK_FORMAT_R32G32B32_SFLOAT:
				attachment._clear_color = {0.0, 0.0, 0.0};
				break;
			case VK_FORMAT_R32G32B32A32_SFLOAT:
				attachment._clear_color = {0.0, 0.0, 0.0, 1.0};
				break;
			case VK_FORMAT_R8G8B8A8_SRGB:
			case VK_FORMAT_R8G8B8A8_UNORM:
				attachment._clear_color = {0, 0, 0, 1};
				break;
			default:
				log_error() << "Unimplimented image format: " << image.format() << std::endl;
				break;
		}

		return attachment;
	}

	util::Result<VkAttachmentDescription, Error> FrameAttachment::attachment_description() const {
		if (_image == nullptr) {
			return Error(
				ErrorType::INVALID_ARG,
				"Cannot get attachment_description from FrameAttachment which is not initialized"
			);
		}

		auto description = VkAttachmentDescription{};
		description.format = _image->format();
		description.samples = VK_SAMPLE_COUNT_1_BIT;
		description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		description.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
		description.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

		return description;
	}

	util::Result<VkPipelineColorBlendAttachmentState, Error> FrameAttachment::blend_attachment_state() const {
		if (_image == nullptr) {
			return Error(
				ErrorType::INVALID_ARG,
				"Cannot get attachment_description from FrameAttachment which is not initialized"
			);
		}

		auto blend_attachment = VkPipelineColorBlendAttachmentState{};
		switch (_image->format()) {
			case VK_FORMAT_R8G8B8A8_SRGB:
				blend_attachment.colorWriteMask =
					VK_COLOR_COMPONENT_R_BIT |
					VK_COLOR_COMPONENT_G_BIT |
					VK_COLOR_COMPONENT_B_BIT |
					VK_COLOR_COMPONENT_A_BIT;
				blend_attachment.blendEnable = VK_TRUE;
				blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
				blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
				break;
			case VK_FORMAT_R16_UINT:
				blend_attachment.colorWriteMask =
					VK_COLOR_COMPONENT_R_BIT |
					VK_COLOR_COMPONENT_A_BIT;
				blend_attachment.blendEnable = VK_FALSE;
				break;
			default:
				return Error(
					ErrorType::SHADER_RESOURCE,
					util::f("Color blend attachment state is not known for image format ", _image->format())
				);
		}

		return blend_attachment;
	}

	VkClearValue FrameAttachment::clear_color() const {
		return _clear_color;
	}

	VkImageView FrameAttachment::image_view() const {
		return _image->image_view();
	}

	VkExtent2D FrameAttachment::size() const {
		return _image->size();
	}
}
