#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "../util/result.hpp"
#include "../util/errors.hpp"
#include "../util/Observer.hpp"
#include "../types/ResourceManager.hpp"
#include "../types/Camera.hpp"
#include "PrevPass.hpp"
#include "RayPass.hpp"
#include "../types/Node.hpp"

namespace vulkan {
	/**
	 * @brief The primary collection of nodes. Can render as a preview or final,
	 * raytraced image
	 */
	class Scene {
		public:
			using Ptr = std::unique_ptr<Scene>;

			Scene(types::ResourceManager &resource_manager, PrevPass::Ptr preview_render_pass);

			Scene(const Scene& other) = delete;
			Scene(Scene &&other) = default;
			Scene& operator=(const Scene& other) = delete;
			Scene& operator=(Scene&& other) = default;

			static util::Result<Ptr, KError> create(types::ResourceManager &resource_manager);

			VkDescriptorSet imgui_descriptor_set();
			ImageView const &image_view();

			VkExtent2D size() const;
			void resize(VkExtent2D new_size);

			void set_is_preview(bool is_preview);
			void render_preview();
			void render_raytrace();

			types::Camera& camera() { return _camera; }
			types::Camera const& camera() const { return _camera; }
			Node const *get_node(uint32_t id) const;
			Node *get_node_mut(uint32_t id);
			void update_node(uint32_t id) { 
				for (auto obs : _node_observers) {
					obs->obs_update(id);
				}
			}

			util::Result<uint32_t, KError> add_node(
					types::Mesh const *mesh,
					types::Material const *material);
			//TODO: removing, identifiying node
			types::ResourceManager &resource_manager();

			util::Result<void, KError> add_node_observer(util::Observer *observer);
			util::Result<void, KError> rem_node_observer(util::Observer *observer);

		private:
			Scene() = default;
			uint32_t _get_node_id();

		private:
			PrevPass::Ptr _preview_render_pass;
			RayPass::Ptr _raytrace_render_pass;

			std::vector<Node::Ptr> _nodes;
			types::Camera _camera;
			types::ResourceManager *_resource_manager;
			bool _is_preview;
			std::list<util::Observer *> _node_observers;
	};
}
