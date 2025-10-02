#include "Node.hpp"
#include "util/log.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace vulkan {
	Node::Ptr Node::create(
		uint32_t id,
		const types::Mesh &mesh,
		const types::Material &material
	) {
		auto result = Ptr(new Node(id, Type::Object));
		//The default name
		result->name() = "Node " + std::to_string(id);
		result->set_mesh(mesh);
		result->set_material(material);

		return std::move(result);
	}

	Node::Ptr Node::create_virtual(uint32_t id) {
		auto result = Ptr(new Node(id, Type::Virtual));
		//The default name
		result->name() = "Node " + std::to_string(id);
		return std::move(result);
	}

	std::string const &Node::name() const {
		return _name;
	}

	std::string &Node::name() {
		return _name;
	}

	Node::Type Node::type() const {
		return _type;
	}

	types::Mesh const &Node::mesh() const {
		log_assert(
			_type == Type::Object,
			"Cannot get the mesh of a non-renderable node. "
			"Make sure you check type() first."
		);
		return *_mesh;
	}

	void Node::set_mesh(types::Mesh const &mesh) {
		log_assert(
			_type == Type::Object,
			"Cannot set mesh of a non-renderable node. "
			"Make sure you check type() first."
		);
		_dirty_bit = true;
		_mesh = &mesh;
	}

	types::Material const &Node::material() const {
		log_assert(
			_type == Type::Object,
			"Cannot get the material of a non-renderable node. "
			"Make sure you check type() first."
		);
		return *_material;
	}

	void Node::set_material(types::Material const &material) {
		log_assert(
			_type == Type::Object,
			"Cannot set material on a non-renderable node. "
			"Make sure you check type() first."
		);
		_dirty_bit = true;
		_material = &material;

		_resources = types::ShaderResources(&material.resources());

		_resources.set_uint32("node_id", _id);
		_resources.set_mat4("object_transformation", get_matrix());
	}

	uint32_t Node::id() const {
		return _id;
	}

	Node *Node::parent() {
		return _parent;
	}

	Node const *Node::parent() const {
		return _parent;
	}

	Node::Container Node::children() {
		return _children;
	}

	Node::Container const Node::children() const {
		return _children;
	}

	void Node::move_to(Node *parent) {
		if (_parent && parent && _parent->id() == parent->id()) return;
		if (!parent && !_parent) return;

		//TODO: Calculate translation relative to parent

		if (_parent) {
			_parent->remove_child(id());
		}
		if (parent) {
			parent->_children.push_back(this);
		}
		_parent = parent;
		
		//set_position(og_position - abs_position());
		//set_rotation(og_rotation - abs_rotation());
		//set_scale(og_scale / abs_scale());
		_propagate_matrix();
	}

	void Node::add_child(Node *child) {
		child->move_to(this);
	}

	void Node::remove_child(Node *child) {
		if (!child) return;
		remove_child(child->id());
	}

	void Node::remove_child(uint32_t id) {
		for (auto c = _children.begin(); c != _children.end(); c++) {
			auto &child = *(*c);
			if (child.id() == id) {
				child._parent = nullptr;
				_children.erase(c);

				_dirty_bit = true;
				child._dirty_bit = true;
				return;
			}
		}
	}

	Node::iterator Node::begin() {
		return _children.begin();
	}

	Node::iterator Node::end() {
		return _children.end();
	}

	Node::const_iterator Node::begin() const {
		return _children.begin();
	}

	Node::const_iterator Node::end() const {
		return _children.end();
	}

	glm::vec3 Node::position() const {
		return _position;
	}

	void Node::set_position(glm::vec3 position) {
		if (position == _position) return;
		_position = position;
		_resources.set_vec3("position", position);
		_propagate_matrix();
	}

	glm::vec3 Node::rotation() const {
		return _rotation;
	}

	void Node::set_rotation(glm::vec3 rotation) {
		if (rotation == _rotation) return;
		_rotation = rotation;
		_propagate_matrix();
	}

	glm::vec3 Node::scale() const {
		return _scale;
	}

	void Node::set_scale(glm::vec3 scale) {
		if (scale == _scale) return;
		_scale = scale;
		_propagate_matrix();
	}

	glm::mat4 Node::get_matrix() const {
		auto result = glm::mat4(1.0);
		if (_parent) {
			result *=_parent->get_matrix();
		}
		result = glm::translate(result, _position);
		result *= glm::eulerAngleYXZ(_rotation.y, _rotation.x, _rotation.z);
		result = glm::scale(result, _scale);
		return result;
	}

	glm::mat4 Node::get_matrix_inverse() const {
		auto s = _scale;
		auto small = 0.000001;
		//Get rid of the divide by 0 error
		if (s.x == 0) { s.x = small; }
		if (s.y == 0) { s.y = small; }
		if (s.z == 0) { s.z = small; }

		auto result = glm::mat4(1.0);
		result = glm::scale(result, glm::vec3(1.0) / s);
		result *= glm::inverse(glm::eulerAngleYXZ(_rotation.y, _rotation.x, _rotation.z));
		result = glm::translate(result, -_position);
		return result;
	}

	types::ShaderResources const &Node::resources() const {
		return _resources;
	}

	types::ShaderResources &Node::resources() {
		return _resources;
	}

	bool Node::dirty_bits() const {
		return _resources.dirty_bits() || _dirty_bit;
	}

	void Node::clear_dirty_bits() {
		_resources.clear_dirty_bits();
		_dirty_bit = false;
	}

	Node::Node(uint32_t id, Node::Type type):
		_id(id),
		_type(type),
		_mesh(nullptr),
		_material(nullptr)
	{
	}

	void Node::_propagate_matrix() {
		_dirty_bit = true;
		_resources.set_mat4("object_transformation", get_matrix());
		for (auto &child : _children) {
			child->_propagate_matrix();
		}
	}

	glm::mat4 Node::rotation_matrix() const {
		auto result = glm::mat4(1.0);
		if (_parent) {
			result *= _parent->rotation_matrix();
		}
		result *= glm::eulerAngleYXZ(_rotation.y, _rotation.x, _rotation.z);
		return result;
	}
}
