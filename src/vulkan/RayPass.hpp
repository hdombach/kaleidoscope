#pragma once

#include <memory>

#include "DescriptorPool.hpp"
#include "DescriptorSet.hpp"
#include "Fence.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include "Semaphore.hpp"
#include "Texture.hpp"
#include "UniformBufferObject.hpp"
#include "RayPassMesh.hpp"
#include "RayPassNode.hpp"

#include "../types/Node.hpp"

#include "../util/Observer.hpp"

namespace vulkan {
	class Scene;

	class RayPass: public Texture {
		public:
			using Ptr = std::unique_ptr<RayPass>;

			class MeshObserver: public util::Observer {
				public:
					MeshObserver() = default;
					MeshObserver(RayPass &ray_pass);
					void obs_create(uint32_t id) override;
					void obs_update(uint32_t id) override;
					void obs_remove(uint32_t id) override;
				private:
					RayPass *_ray_pass;
			};

			class NodeObserver: public util::Observer {
				public:
					NodeObserver() = default;
					NodeObserver(RayPass &ray_pass);
					void obs_create(uint32_t id) override;
					void obs_update(uint32_t id) override;
					void obs_remove(uint32_t id) override;
				private:
					RayPass *_ray_pass;
			};

			static util::Result<Ptr, KError> create(Scene &scene, VkExtent2D size);

			RayPass(const RayPass& other) = delete;
			RayPass(RayPass &&other);
			RayPass& operator=(const RayPass& other) = delete;
			RayPass& operator=(RayPass&& other);

			RayPass();

			void destroy();
			~RayPass();

			VkDescriptorSet get_descriptor_set() override;
			ImageView const &image_view() override;
			void submit(Node &node);
			MappedComputeUniform &current_uniform_buffer();

			MeshObserver &mesh_observer() { return _mesh_observer; }
			NodeObserver &node_observer() { return _node_observer; }

		private:
			void mesh_create(uint32_t id);
			void mesh_update(uint32_t id);
			void mesh_remove(uint32_t id);

			void node_create(uint32_t id);
			void node_update(uint32_t id);
			void node_remove(uint32_t id);

		private:
			VkExtent2D _size;
			Image _result_image;
			ImageView _result_image_view;
			Fence _pass_fence;
			Semaphore _pass_semaphore;
			DescriptorPool _descriptor_pool;
			DescriptorSets _descriptor_set;
			VkDescriptorSet _imgui_descriptor_set;
			VkPipelineLayout _pipeline_layout;
			VkPipeline _pipeline;
			VkCommandBuffer _command_buffer;
			MappedComputeUniform _mapped_uniform;
			MeshObserver _mesh_observer;
			NodeObserver _node_observer;

			VkBuffer _vertex_buffer;
			VkDeviceMemory _vertex_buffer_memory;
			VkDeviceSize _vertex_buffer_range;

			VkBuffer _index_buffer;
			VkDeviceMemory _index_buffer_memory;
			VkDeviceSize _index_buffer_range;

			VkBuffer _mesh_buffer;
			VkDeviceMemory _mesh_buffer_memory;
			VkDeviceSize _mesh_buffer_range;

			VkBuffer _node_buffer;
			VkDeviceMemory _node_buffer_memory;
			VkDeviceSize _node_buffer_range;

			std::vector<RayPassMesh> _meshes;
			std::vector<RayPassNode> _nodes;

			Scene *_scene;

			util::Result<void, KError> _create_descriptor_sets();
			util::Result<void, KError> _create_pipeline();

			void _create_mesh_buffers();
			void _destroy_mesh_buffers();
			void _create_node_buffers();
			void _destroy_node_buffers();
			static void _create_comp_buffer(
					void *data,
					VkBuffer &buffer,
					VkDeviceMemory &buffer_memory,
					VkDeviceSize range);
	};
}
