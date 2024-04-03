#pragma once

#include <vector>

#include <vulkan/vulkan_core.h>

#include "../util/result.hpp"
#include "../util/errors.hpp"
#include "PreviewRenderPass.hpp"
#include "Node.hpp"
#include "../types/resourceManager.hpp"

namespace vulkan {
	/**
	 * @brief The primary collection of nodes. Can render as a preview or final,
	 * raytraced image
	 */
	class Scene {
		public:
			Scene(PreviewRenderPass::Ptr preview_render_pass);

			static util::Result<Scene, KError> create(types::ResourceManager &resource_manager);

			VkExtent2D size() const;
			void resize(VkExtent2D new_size);

			void render_preview();
			Texture& preview_texture();

			util::Result<void, KError> add_node(Node node);
			//TODO: removing, identifiying node

		private:
			PreviewRenderPass::Ptr _preview_render_pass;

			std::vector<Node> _nodes;
	};
}
