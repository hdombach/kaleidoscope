#pragma once

#include "vulkan/DescriptorSet.hpp"
#include "vulkan/Error.hpp"
#include "vulkan/StaticBuffer.hpp"
#include "util/result.hpp"
#include "util/UIDList.hpp"
#include "types/Mesh.hpp"
#include "types/Node.hpp"
#include "vulkan/TemplUtils.hpp"

namespace vulkan {
	class Scene;
	class InstancedPass;

	/**
	 * @brief Handles mesh related data for the InstancedPass
	 * Stores the vertex and index buffers needed for drawing the mesh.
	 * Also keeps track of all the nodes that use this mesh so it can render
	 * them together.
	 */
	class InstancedPassMesh {
		public:
			/**
			 * @brief Values that are used per instance of this mesh that is used
			 *
			 * The node_id is assumed based off where the node is in the buffer
			 */
			struct NodeVImpl {
				alignas(4) uint32_t node_id;
				alignas(4) uint32_t material_id;
				alignas(16) glm::vec3 position;
				alignas(16) glm::mat4 transformation;
				alignas(16) glm::mat4 inverse_transformation;

				NodeVImpl();
				NodeVImpl(vulkan::Node const &node);

				/**
				 * @brief Information for codegenerating glsl struct
				 */
				inline const static auto declaration = std::vector{
					templ_property("uint", "node_id"),
					templ_property("uint", "material_id"),
					templ_property("vec3", "position"),
					templ_property("mat4", "transformation"),
					templ_property("mat4", "inverse_transformation"),
				};
			} __attribute__((packed));

		public:
			InstancedPassMesh() = default;

			static util::Result<InstancedPassMesh, Error> create(
				const types::Mesh *mesh,
				InstancedPass const &instanced_pass
			);

			InstancedPassMesh(InstancedPassMesh const &other) = delete;
			InstancedPassMesh(InstancedPassMesh &&other);
			InstancedPassMesh &operator=(InstancedPassMesh const &other) = delete;
			InstancedPassMesh &operator=(InstancedPassMesh &&other);

			bool has_value() const;
			operator bool() const;

			/**
			 * @brief Is the mesh a an actual mesh or a DE field
			 */
			bool is_de() const;

			/**
			 * @brief The underlying mesh id
			 */
			uint32_t id() const;
			/**
			 * @brief Vulkan buffer containing vertexes. Is already indexed
			 * using index buffer
			 */
			StaticBuffer const &vertex_buffer() const;
			/**
			 * @brief Index buffer for referencing vertexes in the vertex buffer. Used
			 * to reduce reuse of vertex points in the buffer
			 */
			StaticBuffer const &index_buffer() const;
			/**
			 * @brief Buffer containing per instance data for each node that is rendered
			 */
			StaticBuffer const &node_buffer() const;

			/**
			 * @brief The number of indices to be rendered
			 */
			uint32_t index_count() const;

			/**
			 * @brief The number of instances to be rendered
			 */
			uint32_t instance_count() const;

			/**
			 * @brief Allows this object to keep track of another node that uses the
			 * underlying mesh.
			 */
			void add_node(vulkan::Node const &node);
			/**
			 * @brief Lets this object know another node is no longer using the
			 * underlying mesh.
			 */
			void remove_node(uint32_t node_id);

			/**
			 * @brief One of the properites of the node was changed
			 */
			util::Result<void, Error> update_node(vulkan::Node const &node);

			/**
			 * @brief Descriptor set that is used per mesh
			 */
			DescriptorSets &descriptor_set();

			void destroy();
			~InstancedPassMesh();

		private:
			/**
			 * @brief Recreates the per instance node data
			 */
			util::Result<void, Error> _create_node_buffer();

		private:
			/**
			 * @brief Base mesh that needs to be implimented
			 */
			const types::Mesh *_mesh;

			InstancedPass const *_instanced_pass;

			/**
			 * @brief The per instance data that will be sent to the shader.
			 */
			std::vector<NodeVImpl> _nodes;

			StaticBuffer _vertex_buffer;
			StaticBuffer _index_buffer;
			StaticBuffer _node_buffer;
			DescriptorSets _descriptor_set;
			uint32_t _index_count;
	};
}
