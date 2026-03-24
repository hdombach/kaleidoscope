#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "types/Node.hpp"
#include "vulkan/TemplUtils.hpp"

namespace vulkan {
	/**
	 * @brief Keeps track of what mesh a node is using
	 * The InstancedPass handles the synchronization
	 */
	class InstancedPassNode {
		public:
			struct VImpl {
				alignas(4) uint32_t mesh_id;
				alignas(4) uint32_t material_id;
				alignas(4) uint32_t is_de;
				alignas(16) glm::vec3 position;
				alignas(16) glm::mat4 tansformation;
				alignas(16) glm::mat4 inverse_transformation;

				static VImpl create_empty();

				inline const static auto declaration = std::vector{
					templ_property("uint", "mesh_id"),
					templ_property("uint", "material_id"),
					templ_property("uint", "is_de"),
					templ_property("vec3", "position"),
					templ_property("mat4", "transformation"),
					templ_property("mat4", "inverse_transformation"),
				};
			} __attribute__((packed));

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

			VImpl vimpl() const;

			void destroy();
			~InstancedPassNode();

			uint32_t registered_mesh = 0;

		private:
			vulkan::Node const *_node = nullptr;
	};
}
