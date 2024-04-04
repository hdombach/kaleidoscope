#pragma once

#include "Material.hpp"
#include "Mesh.hpp"

namespace vulkan {
	/**
	 * @brief A generic thing that can be rendered in a Scene
	 */
	class Node {
		public:
			Node(Mesh &mesh, Material &material):
				_mesh(mesh),
				_material(material)
			{}

			Mesh &mesh() { return _mesh; }
			Mesh const &mesh() const { return _mesh; }

			Material &material() { return _material; }
			Material const &material() const { return _material; }

		private:
			Mesh &_mesh;
			Material &_material;
	};
}
