#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "Material.hpp"
#include "../types/Mesh.hpp"

namespace vulkan {
	/**
	 * @brief A generic thing that can be rendered in a Scene
	 */
	class Node {
		public:
			Node(types::Mesh const &mesh, Material const &material):
				_mesh(mesh),
				_material(material),
				_position(0, 0, 0)
			{}

			Node(const Node& other) = delete;
			Node(Node &&other);
			Node& operator=(const Node& other) = delete;
			Node& operator=(Node&& other) = delete;

			types::Mesh const &mesh() const { return _mesh; }

			Material const &material() const { return _material; }

			glm::vec3 position() const { return _position; };
			void set_position(glm::vec3 position) { _position = position; };

			void render_preview(
					PrevPass &preview_render_pass,
					VkCommandBuffer command_buffer);

		private:
			types::Mesh const &_mesh;
			vulkan::Material const &_material;

			glm::vec3 _position;
	};
}
