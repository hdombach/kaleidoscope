#pragma once

#include <list>
#include <memory>

#include <vulkan/vulkan_core.h>

#include "util/result.hpp"
#include "util/errors.hpp"
#include "util/Observer.hpp"
#include "util/Util.hpp"
#include "types/ResourceManager.hpp"
#include "types/Camera.hpp"
#include "prev_pass/PrevPass.hpp"
#include "ray_pass/RayPass.hpp"
#include "types/Node.hpp"

namespace vulkan {
	/**
	 * @brief The primary collection of nodes. Can render as a preview or final,
	 * raytraced image
	 */
	class Scene {
		public:
			using Ptr = std::unique_ptr<Scene>;
			using Container = util::UIDList<Node::Ptr, util::has_value, util::id_deref_trait>;
			using iterator = Container::iterator;
			using const_iterator = Container::const_iterator;

			Scene(types::ResourceManager &resource_manager, PrevPass::Ptr preview_render_pass);

			Scene(const Scene& other) = delete;
			Scene(Scene &&other) = default;
			Scene& operator=(const Scene& other) = delete;
			Scene& operator=(Scene&& other) = default;

			static util::Result<Ptr, KError> create(types::ResourceManager &resource_manager);

			VkDescriptorSet imgui_descriptor_set();
			VkImageView image_view();

			VkExtent2D size() const;
			void resize(VkExtent2D new_size);

			int render_rate() const;
			void set_render_rate(int rate);
			void set_is_preview(bool is_preview);
			VkSemaphore render_preview(VkSemaphore semaphore);
			VkSemaphore render_raytrace(VkSemaphore semaphore);
			void update();

			void set_selected_node(uint32_t n) { _selected_node = n; }
			uint32_t selected_node() { return _selected_node; }
			void set_camera(types::Camera const &camera);
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
			util::Result<void, KError> rem_node(uint32_t id);
			//TODO: removing, identifiying node
			types::ResourceManager &resource_manager();

			util::Result<void, KError> add_node_observer(util::Observer *observer);
			util::Result<void, KError> rem_node_observer(util::Observer *observer);

			iterator begin() {
				return _nodes.begin();
			}
			iterator end() {
				return _nodes.end();
			}
			const_iterator begin() const {
				return _nodes.begin();
			}
			const_iterator end() const {
				return _nodes.end();
			}

		private:
			Scene() = default;

		private:
			PrevPass::Ptr _preview_render_pass;
			RayPass::Ptr _raytrace_render_pass;

			/**
			 * @brief List of abstract nodes
			 * First element is reserved for unused id
			 */
			Container _nodes;
			types::Camera _camera;
			bool _camera_dirty_bit = false;
			types::ResourceManager *_resource_manager;
			bool _is_preview;
			int _render_rate = 10000;
			std::list<util::Observer *> _node_observers;
			uint32_t _selected_node;
	};
}
