#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "types/Node.hpp"

namespace vulkan {
	/**
	 * @brief Keeps track of what mesh a node is using
	 * The InstancedPass handles the synchronization
	 */
	class InstancedPassNode {
		public:
			/**
			 * @brief Creates an empty InstancedPassNode
			 */
			InstancedPassNode() = default;

			/**
			 * @brief Creates an InstancedPassNode form a node
			 */
			static InstancedPassNode create(vulkan::Node const &node);

			InstancedPassNode(InstancedPassNode const &other) = delete;
			InstancedPassNode(InstancedPassNode &&other);
			InstancedPassNode &operator=(InstancedPassNode const &other) = delete;
			InstancedPassNode &operator=(InstancedPassNode &&other);

			bool has_value() const;
			operator bool() const;

			/**
			 * @brief The underlying node id
			 */
			uint32_t id() const;
			types::Mesh const &mesh() const;
			vulkan::Node const &node() const;

			void destroy();
			~InstancedPassNode();

			uint32_t registered_mesh = 0;

		private:
			vulkan::Node const *_node = nullptr;
	};
}
