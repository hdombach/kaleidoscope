#pragma once

#include <cstdint>
#include <vector>
#include <ostream>

#include "types/Mesh.hpp"
//#include "BVNode.hpp"

namespace vulkan {
	class RayPass;
	class BVNode;
	/**
	 * @brief Contains reference to a BVNode buffer in the RayPass
	 */
	class RayPassMesh {
		public:
			RayPassMesh(
					const types::Mesh *mesh,
					RayPass *ray_pass):
				_mesh(mesh),
				_ray_pass(ray_pass)
			{ }

			bool has_value() const { return _mesh; }
			operator bool() const { return has_value(); }

			/**
			 * @brief Populates nodes and vertices with data corresponding to underlying mesh
			 * @param[out] nodes
			 * @param[out] vertices
			 */
			void build(std::vector<BVNode> &nodes, std::vector<Vertex> &vertices);

			/**
			 * @brief Root bvnode index in the RayPass node buffer
			 */
			uint32_t bvnode_id() const { return _bvnode_id; }

			/**
			 * @brief Underlying generic mesh
			 */
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

	/**
	 * @brief Vulkan representation of bounding volume hierarchy node
	 */
	struct BVNode {
		alignas(16) glm::vec3 min_pos;
		alignas(16) glm::vec3 max_pos;
		alignas(4)  BVType type;
		/**
		 * @brief Either bvnode or triangle index
		 */
		alignas(4)  uint32_t lchild;
		/**
		 * @brief Either bvnode or triangle index
		 */
		alignas(4)  uint32_t rchild;
		/**
		 * @brief Parent bvnode
		 * Is 0 if it is root node
		 */
		alignas(4)  uint32_t parent;

		/**
		 * @brief Creates node with type Unknown
		 */
		static BVNode create_empty();

		/**
		 * @brief Vulkan source code declaration for bvnode
		 */
		static constexpr const char *declaration() {
			return
			"#define BV_UNKNOWN 0\n"
			"#define BV_MESH 1\n"
			"#define BV_NODE 2\n"
			"struct BVNode {\n"
			"\tvec3 min_pos;\n"
			"\tvec3 max_pos;\n"
			"\tuint type;\n"
			"\tuint lchild;\n"
			"\tuint rchild;\n"
			"\tuint parent;\n"
			"};\n";
		}

		std::ostream& print_debug(std::ostream& os) const;
	} __attribute__((packed));

	/**
	 * @brief Intermediate class for building bounding volume hierarchy
	 * Building the tree takes three seperate steps
	 * - Add all vertices
	 * - Recursively split into smaller bvnodes
	 * - Copy bvnodes and vertices to main buffer
	 */
	class BVNodeBuilder {
		public:
			BVNodeBuilder(): _is_leaf(false) {}

			/**
			 * @brief Adds vertex to internal buffer and keeps track of bounding box
			 * @param[in] v Vertex to add
			 */
			void add_vertex(Vertex v);
			/**
			 * @brief Recursively split into smaller bvnodes
			 * - Create two child nodes based on which axis is biggest
			 * - Copies each triange to child node based on midpoint
			 */
			void split();
			/**
			 * @brief Populates nodes and vertices buffers
			 * @param[out] nodes
			 * @param[out] vertices
			 * @returns Index current bvnode in node buffer
			 */
			uint32_t build(std::vector<BVNode> &nodes, std::vector<Vertex> &vertices);
			/**
			 * @brief Average position of all vertices
			 */
			glm::vec3 avg_pos() const;
			std::ostream& print_debug(std::ostream& os) const;

		private:
			std::vector<Vertex> _verts;
			bool _is_leaf;
			glm::vec3 _min_pos;
			glm::vec3 _max_pos;
			glm::vec3 _pos_sum;
			std::unique_ptr<BVNodeBuilder> _lchild;
			std::unique_ptr<BVNodeBuilder> _rchild;

			void _normalize_children();
	};
}

inline std::ostream& operator<<(std::ostream& os, vulkan::BVType const &type) {
	switch (type) {
		case vulkan::BVType::Unknown:
			return os << "Unknown";
		case vulkan::BVType::Mesh:
			return os << "Mesh";
		case vulkan::BVType::Node:
			return os << "Node";
	}
}


inline std::ostream& operator<<(std::ostream& os, vulkan::BVNode const &node) {
	return node.print_debug(os);
}

inline std::ostream& operator<<(
		std::ostream& os,
		vulkan::BVNodeBuilder const &node)
{
	return node.print_debug(os);
}
