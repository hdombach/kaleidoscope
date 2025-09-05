#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "Material.hpp"
#include "ShaderResource.hpp"
#include "Mesh.hpp"

namespace vulkan {
	/**
	 * @brief A generic thing that can be rendered in a Scene
	 */
	class Node {
		public:
			using Ptr = std::unique_ptr<Node>;
			using Container = std::vector<Node *>;
			using iterator = Container::iterator;
			using const_iterator = Container::const_iterator;

			static Ptr create(
				uint32_t id,
				types::Mesh const &mesh,
				types::Material const &material
			);

			static Ptr create_virtual(uint32_t id);

			Node(const Node& other) = delete;
			Node(Node &&other) = default;
			Node& operator=(const Node& other) = delete;
			Node& operator=(Node&& other) = delete;

			std::string const &name() const;
			std::string &name();

			types::Mesh const &mesh() const;
			void set_mesh(types::Mesh const &mesh);

			types::Material const &material() const;
			void set_material(types::Material const &material);

			bool is_virtual() const;

			uint32_t id() const;

			Node *parent();
			Node const *parent() const;
			Container children();
			Container const children() const;

			void move_to(Node *parent);
			void add_child(Node *child);
			void remove_child(Node *child);
			void remove_child(uint32_t id);

			iterator begin();
			iterator end();
			const_iterator begin() const;
			const_iterator end() const;

			glm::vec3 position() const;
			void set_position(glm::vec3 position);
			glm::vec3 rotation() const;
			void set_rotation(glm::vec3 rotation);
			glm::vec3 scale() const;
			void set_scale(glm::vec3 scale);
			glm::mat4 get_matrix() const;
			glm::mat4 get_matrix_inverse() const;
			types::ShaderResources const &resources() const;
			types::ShaderResources &resources();

			bool dirty_bits() const;
			void clear_dirty_bits();

		private:
			uint32_t _id;
			bool _is_virtual;
			types::Mesh const *_mesh=nullptr;;
			types::Material const *_material=nullptr;

			std::string _name;

			// The nodes are owned by the Scene so that the scene can keep track
			// of uid's and observers
			Node *_parent;
			Container _children;

			glm::vec3 _position;
			glm::vec3 _rotation;
			glm::vec3 _scale = glm::vec3(1.0);

			types::ShaderResources _resources;
			
			bool _dirty_bit;

			Node(uint32_t id):
				_id(id),
				_mesh(nullptr),
				_material(nullptr)
		{}
	};
}
