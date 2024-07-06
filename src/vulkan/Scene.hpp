#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "../util/result.hpp"
#include "../util/errors.hpp"
#include "PrevPass.hpp"
#include "RaytraceRenderPass.hpp"
#include "Node.hpp"
#include "../types/ResourceManager.hpp"
#include "../types/Camera.hpp"

namespace vulkan {
	/**
	 * @brief The primary collection of nodes. Can render as a preview or final,
	 * raytraced image
	 */
	class Scene: public Texture {
		public:
			using Ptr = std::unique_ptr<Scene>;

			Scene(types::ResourceManager &resource_manager, PrevPass::Ptr preview_render_pass);

			Scene(const Scene& other) = delete;
			Scene(Scene &&other) = default;
			Scene& operator=(const Scene& other) = delete;
			Scene& operator=(Scene&& other) = default;

			static util::Result<Ptr, KError> create(types::ResourceManager &resource_manager);

			VkDescriptorSet get_descriptor_set() override;
			ImageView const &image_view() override;

			VkExtent2D size() const;
			bool is_resizable() const override { return true; }
			void resize(VkExtent2D new_size) override;

			void set_is_preview(bool is_preview);
			void render_preview();
			void render_raytrace();

			types::Camera& camera() { return _camera; }
			types::Camera const& camera() const;

			util::Result<void, KError> add_node(Node node);
			//TODO: removing, identifiying node
			types::ResourceManager &resource_manager();

		private:
			Scene() = default;
			Texture& _cur_texture();

			PrevPass::Ptr _preview_render_pass;
			RaytraceRenderPass::Ptr _raytrace_render_pass;

			std::vector<Node> _nodes;
			types::Camera _camera;
			types::ResourceManager *_resource_manager;
			bool _is_preview;
	};
}
