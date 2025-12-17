#pragma once

#include <list>
#include <memory>

#include <vulkan/vulkan_core.h>

#include "util/result.hpp"
#include "util/Observer.hpp"
#include "util/Util.hpp"
#include "util/map_iterator.hpp"
#include "util/IterAdapter.hpp"
#include "types/ResourceManager.hpp"
#include "types/Camera.hpp"
#include "prev_pass/PrevPass.hpp"
#include "ray_pass/RayPass.hpp"
#include "types/Node.hpp"
#include "Error.hpp"

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

			using camera_iterator = util::filter_iterator<util::map_iterator<iterator, types::Camera*>>;

			Scene(types::ResourceManager &resource_manager, PrevPass::Ptr preview_render_pass);

			Scene(const Scene& other) = delete;
			Scene(Scene &&other) = default;
			Scene& operator=(const Scene& other) = delete;
			Scene& operator=(Scene&& other) = default;

			static util::Result<Ptr, Error> create(types::ResourceManager &resource_manager);

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
			void set_active_camera(uint32_t id);
			/* Get id of active camera. Is 0 if node based camera is not selected */
			uint32_t camera_id();
			types::Camera &camera();
			types::Camera const& camera() const;
			Node const *get_node(uint32_t id) const;
			Node *get_node_mut(uint32_t id);
			void update_node(uint32_t id) { 
				for (auto obs : _node_observers) {
					obs->obs_update(id);
				}
			}

			util::Result<Node *, Error> create_node(
				types::Mesh const *mesh,
				types::Material const *material
			);
			util::Result<Node *, Error> create_virtual_node();
			util::Result<types::Camera *, Error> create_camera();
			util::Result<void, Error> remove_node(uint32_t id);
			//TODO: removing, identifiying node
			types::ResourceManager &resource_manager();

			Node *root();
			Node const *root() const;

			util::Result<void, Error> add_node_observer(util::Observer *observer);
			util::Result<void, Error> rem_node_observer(util::Observer *observer);

			Container const &nodes() const;
			Container &nodes();

			iterator begin();
			iterator end();
			const_iterator begin() const;
			const_iterator end() const;

			camera_iterator cameras_begin();
			camera_iterator cameras_end();
			inline auto cameras() {
				return util::Adapt(cameras_begin(), cameras_end());
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
			types::Camera::Ptr _viewport_camera;
			Node *_root;
			bool _camera_dirty_bit = false;
			types::ResourceManager *_resource_manager;
			bool _is_preview;
			int _render_rate = 10000;
			std::list<util::Observer *> _node_observers;
			uint32_t _selected_node;
			//active_camera = 0 for freeform camera
			uint32_t _active_camera;
	};
}
