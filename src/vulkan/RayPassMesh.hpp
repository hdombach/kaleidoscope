#pragma once

#include <cstdint>
#include <vector>
#include <ostream>

#include "../types/Mesh.hpp"
//#include "BVNode.hpp"

namespace vulkan {
	class RayPass;
	class BVNode;
	/* Reference to a single buffer that contains every vertice for all the meshes */
	class RayPassMesh {
		public:
			RayPassMesh(
					const types::Mesh *mesh,
					RayPass *ray_pass):
				_mesh(mesh),
				_ray_pass(ray_pass)
			{ }

			void build(std::vector<BVNode> &nodes, std::vector<Vertex> vertices);

			uint32_t bvnode_id() const { return _bvnode_id; }

			const types::Mesh *base_mesh() { return _mesh; }

		private:
			const types::Mesh *_mesh;
			const RayPass *_ray_pass;

			uint32_t _bvnode_id;
	};

	enum class BVType: uint32_t {
		Unknown,
		Mesh,
		Node,
	};

	inline std::ostream& operator>>(std::ostream& os, BVType const &type) {
		switch (type) {
			case BVType::Unknown:
				return os << "Unknown";
			case BVType::Mesh:
				return os << "Mesh";
			case BVType::Node:
				return os << "Node";
		}
	}

	struct BVNode {
		alignas(16) glm::vec3 min_pos;
		alignas(16) glm::vec3 max_pos;
		alignas(4)  BVType type;
		alignas(4)  uint32_t lchild; /* either bvnode or triangle index */
		alignas(4)  uint32_t rchild; /* either bvnode or triangle index */ 
		alignas(4)  uint32_t parent;

		static constexpr const char *declaration() {
			return
			"#define BV_UNKNOWN 0\n"
			"#define BV_MESH 1\n"
			"#define BV_NODE 2\n"
			"struct BVNode {\n"
			"	vec3 min_pos;\n"
			"	vec3 max_pos;\n"
			"	uint type;\n"
			"	uint lchild;\n"
			"	uint rchild;\n"
			"	uint parent;\n"
			"};\n";
		}

		std::ostream& print_debug(std::ostream& os) const;
	} __attribute__((packed));

	class BVNodeBuilder {
		public:
			BVNodeBuilder(): _is_leaf(false) {}

			void add_vertex(Vertex v);
			void split();
			uint32_t build(std::vector<BVNode> &nodes, std::vector<Vertex> &vertices);
			glm::vec3 avg_pos() const;

		private:
			std::vector<Vertex> _verts;
			bool _is_leaf;
			glm::vec3 _min_pos;
			glm::vec3 _max_pos;
			std::unique_ptr<BVNodeBuilder> _lchild;
			std::unique_ptr<BVNodeBuilder> _rchild;

			void _normalize_children();
	};
}

inline std::ostream& operator<<(std::ostream& os, vulkan::BVNode const &node) {
	return node.print_debug(os);
}
