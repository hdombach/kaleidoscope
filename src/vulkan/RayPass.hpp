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
#include "RayPassMaterial.hpp"
#include "StaticBuffer.hpp"

#include "../types/Node.hpp"

#include "../util/Observer.hpp"

namespace vulkan {
	class Scene;

	class RayPass {
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

			class MaterialObserver: public util::Observer {
				public:
					MaterialObserver() = default;
					MaterialObserver(RayPass &ray_pass);
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

			VkDescriptorSet imgui_descriptor_set();
			ImageView const &image_view();
			void submit(Node &node);
			MappedComputeUniform &current_uniform_buffer();

			MeshObserver &mesh_observer() { return _mesh_observer; }
			MaterialObserver &material_observer() { return _material_observer; }
			NodeObserver &node_observer() { return _node_observer; }

		private:
			void mesh_create(uint32_t id);
			void mesh_update(uint32_t id);
			void mesh_remove(uint32_t id);

			void material_create(uint32_t id);
			void material_update(uint32_t id);
			void material_remove(uint32_t id);

			void node_create(uint32_t id);
			void node_update(uint32_t id);
			void node_remove(uint32_t id);

			util::Result<void, KError> _create_descriptor_sets();
			util::Result<void, KError> _create_pipeline();

			void _create_mesh_buffers();
			void _create_node_buffers();
			std::string _codegen();

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
			MaterialObserver _material_observer;
			NodeObserver _node_observer;

			StaticBuffer _vertex_buffer;
			StaticBuffer _index_buffer;
			StaticBuffer _mesh_buffer;
			StaticBuffer _node_buffer;

			std::vector<RayPassMesh> _meshes;
			std::vector<RayPassNode> _nodes;
			std::vector<RayPassMaterial> _materials;

			Scene *_scene;
	};
}
