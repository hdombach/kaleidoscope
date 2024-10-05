#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "Material.hpp"
#include "../types/Mesh.hpp"
#include "ShaderResource.hpp"

namespace vulkan {
	/**
	 * @brief A generic thing that can be rendered in a Scene
	 */
	class Node {
		public:
			using Ptr = std::unique_ptr<Node>;

			static Ptr create(
					uint32_t id,
					types::Mesh const &mesh,
					types::Material const &material)
			{
				auto result = Ptr(new Node(id));
				result->name() = "Node " + std::to_string(id);
				result->set_mesh(mesh);
				result->set_material(material);

				return std::move(result);
			}

			Node(const Node& other) = delete;
			Node(Node &&other) = default;
			Node& operator=(const Node& other) = delete;
			Node& operator=(Node&& other) = delete;

			std::string const &name() const { return _name; }
			std::string &name() { return _name; }

			types::Mesh const &mesh() const { return *_mesh; }
			void set_mesh(types::Mesh const &mesh) {
				_dirty_bity = true;
				_mesh = &mesh;
			}

			types::Material const &material() const { return *_material; }
			void set_material(types::Material const &material) {
				_dirty_bity = true;
				_material = &material;

				_resources = types::ShaderResources(&material.resources());
				_resources.add_resource(types::ShaderResource::create_primitive("position", _position));
				_resources.set_uint32("node_id", _id);
			}

			uint32_t id() const { return _id; }

			glm::vec3 position() const { return _position; };
			void set_position(glm::vec3 position) {
				if (position == _position) return;
				_position = position;
				_resources.set_vec3("position", position);
			}
			types::ShaderResources const &resources() const { return _resources; }
			types::ShaderResources &resources() { return _resources; }

			bool dirty_bits() const {
				return _resources.dirty_bits() || _dirty_bity;
			}
			void clear_dirty_bits() {
				_resources.clear_dirty_bits();
				_dirty_bity = false;
			}

		private:
			uint32_t _id;
			types::Mesh const *_mesh;
			types::Material const *_material;

			std::string _name;

			glm::vec3 _position;

			types::ShaderResources _resources;
			
			bool _dirty_bity;

			Node(uint32_t id):
				_id(id),
				_mesh(nullptr),
				_material(nullptr)
		{}
	};
}
