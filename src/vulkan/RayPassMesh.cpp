#include "RayPassMesh.hpp"
#include "../types/Axis.hpp"
#include "../util/log.hpp"
#include "../util/format.hpp"

namespace vulkan {

	std::ostream& BVNode::print_debug(std::ostream& os) const {
		return os << "{"
			<< "\"min_pos\":" << min_pos
			<< "\"max_pos\":" << max_pos
			<< "}";
	}

	void RayPassMesh::build(
			std::vector<BVNode> &nodes,
			std::vector<Vertex> vertices)
	{
		auto b = BVNodeBuilder();
		for (auto &v : *_mesh) {
			b.add_vertex(v);
		}
		b.split();
		_bvnode_id = b.build(nodes, vertices);
		return;
	}

	void BVNodeBuilder::add_vertex(Vertex v) {
		if (_verts.empty()) {
			_min_pos = v.pos;
			_max_pos = v.pos;
		} else {
			if (v.pos.x < _min_pos.x) {
				_min_pos.x = v.pos.x;
			}
			if (v.pos.y < _min_pos.y) {
				_min_pos.y = v.pos.y;
			}
			if (v.pos.z < _min_pos.z) {
				_min_pos.z = v.pos.z;
			}

			if (v.pos.x > _max_pos.x) {
				_max_pos.x = v.pos.x;
			}
			if (v.pos.y > _max_pos.y) {
				_max_pos.y = v.pos.y;
			}
			if (v.pos.z > _max_pos.z) {
				_max_pos.z = v.pos.z;
			}
		}
	}

	void BVNodeBuilder::split() {
		if (_verts.size() <= 2) {
			_is_leaf = true;
			return;
		}

		auto diff = _max_pos - _min_pos;
		types::Axis axis;
		if (diff.x > diff.y && diff.x > diff.z) {
			axis = types::Axis::X;
		} else if (diff.y > diff.z) {
			axis = types::Axis::Y;
		} else {
			axis = types::Axis::Z;
		}

		_lchild = std::unique_ptr<BVNodeBuilder>();
		_rchild = std::unique_ptr<BVNodeBuilder>();

		auto avg_pos = (_min_pos + _max_pos) / 2.0f;
		for (int i = 0; i < _verts.size(); i += 3) {
			auto v1 = _verts[i + 0];
			auto v2 = _verts[i + 1];
			auto v3 = _verts[i + 2];

			auto avg_vert = (v1.pos + v2.pos + v3.pos) / 3.0f;

			bool is_left;
			if (axis == types::Axis::X) {
				is_left = avg_vert.x < avg_pos.x;
			} else if (axis == types::Axis::Y) {
				is_left = avg_vert.y < avg_pos.y;
			} else {
				is_left = avg_vert.z < avg_pos.z;
			}

			if (is_left) {
				_lchild->add_vertex(v1);
				_lchild->add_vertex(v2);
				_lchild->add_vertex(v3);
			} else {
				_rchild->add_vertex(v1);
				_rchild->add_vertex(v2);
				_rchild->add_vertex(v3);
			}
		}
		_verts.clear();
		//make sure they are relative
		_lchild->_normalize_children();
		_rchild->_normalize_children();
		_lchild->split();
		_rchild->split();
	}

	uint32_t BVNodeBuilder::build(
			std::vector<BVNode> &nodes,
			std::vector<Vertex> &vertices)
	{
		auto res = nodes.size();
		nodes.push_back(BVNode());

		auto &node = nodes[res];

		node.min_pos = _min_pos;
		node.max_pos = _max_pos;
		node.parent = -1;

		if (_is_leaf) {
			node.type = BVType::Mesh;
			if (_verts.size() > 2) {
				LOG_FATAL_ERROR << "BVNodeBuilder has more than 2 children." << std::endl;
				return res;
			}
			auto vstart = vertices.size();
			if (_verts.size() == 0) {
				node.lchild = -1;
				node.rchild = -1;
			} else if (_verts.size() == 1) {
				node.lchild = vstart;
				node.rchild = -1;
			} else if (_verts.size() == 2) {
				node.lchild = vstart;
				node.rchild = vstart + 3;
			}
		} else {
			node.type = BVType::Node;
			node.lchild = _lchild->build(nodes, vertices);
			node.rchild = _rchild->build(nodes, vertices);
			nodes[node.lchild].parent = res;
			nodes[node.rchild].parent = res;
		}
	
		return res;
	}

	void BVNodeBuilder::_normalize_children() {
		for (auto &v : _verts) {
			v.pos -= _min_pos;
		}
	}

}
