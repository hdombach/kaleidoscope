#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "Material.hpp"
#include "Mesh.hpp"

namespace vulkan {
	/**
	 * @brief A generic thing that can be rendered in a Scene
	 */
	class Node {
		public:
			Node(Mesh &mesh, std::unique_ptr<Material> &&material):
				_mesh(mesh),
				_material(std::move(material)),
				_position(0, 0, 0)
			{}

			Node(Mesh &mesh, Material *material):
				_mesh(mesh),
				_material(material),
				_position(0, 0, 0)
			{}

			Node(const Node& other) = delete;
			Node(Node &&other);
			Node& operator=(const Node& other) = delete;
			Node& operator=(Node&& other) = delete;

			Mesh &mesh() { return _mesh; }
			Mesh const &mesh() const { return _mesh; }

			Material &material() { return *_material; }
			Material const &material() const { return *_material; }

			glm::vec3 position() const { return _position; };
			void set_position(glm::vec3 position) { _position = position; };

			void render_preview(
					PreviewRenderPass &preview_render_pass,
					VkCommandBuffer command_buffer);

		private:
			Mesh &_mesh;
			std::unique_ptr<Material> _material;

			glm::vec3 _position;
	};
}