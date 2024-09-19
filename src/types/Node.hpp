#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <string>

#include "Material.hpp"
#include "../types/Mesh.hpp"

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
				auto result = Ptr(new Node(id, mesh, material));
				result->name() = "Node " + std::to_string(id);

				result->_resources.add_resource(types::ShaderResource::create_primitive("position", result->_position));

				return std::move(result);
			}

			Node(const Node& other) = delete;
			Node(Node &&other) = default;
			Node& operator=(const Node& other) = delete;
			Node& operator=(Node&& other) = delete;

			std::string const &name() const { return _name; }
			std::string &name() { return _name; }

			types::Mesh const &mesh() const { return _mesh; }

			types::Material const &material() const { return _material; }

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
				return _resources.dirty_bits();
			}
			void clear_dirty_bits() {
				_resources.clear_dirty_bits();
			}

		private:
			uint32_t _id;
			types::Mesh const &_mesh;
			types::Material const &_material;

			std::string _name;

			glm::vec3 _position;

			types::ShaderResources _resources;

			Node(uint32_t id, types::Mesh const &mesh, types::Material const &material):
				_id(id),
				_mesh(mesh),
				_material(material)
		{}
	};
}
