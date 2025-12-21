#pragma once

#include <cstdint>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include "vulkan/TemplUtils.hpp"
#include "vulkan/Error.hpp"
#include "types/Node.hpp"

namespace vulkan {
	class Scene;
	class InstancedPass;

	class InstancedPassNode {
		public:
			struct VImpl {
				alignas(4) uint32_t mesh_id;
				alignas(4) uint32_t material_id;
				alignas(16) glm::vec3 position;
				alignas(16) glm::mat4 transformation;
				alignas(16) glm::mat4 inverse_transformation;

				static VImpl create_empty();

				inline const static auto declaration = std::vector{
					templ_property("uint", "mesh_id"),
					templ_property("uint", "material_id"),
					templ_property("vec3", "position"),
					templ_property("mat4", "transformation"),
					templ_property("mat4", "inverse_transformation"),
				};
			} __attribute__((packed));

			InstancedPassNode() = default;

			static InstancedPassNode create(vulkan::Node const &node, InstancedPass &instanced_pass);

			InstancedPassNode(InstancedPassNode const &other) = delete;
			InstancedPassNode(InstancedPassNode &&other);
			InstancedPassNode &operator=(InstancedPassNode const &other) = delete;
			InstancedPassNode &operator=(InstancedPassNode &&other);

			bool has_value() const;
			operator bool() const;

			void destroy();
			~InstancedPassNode();
	};
}
