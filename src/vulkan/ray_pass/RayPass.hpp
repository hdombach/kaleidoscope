#pragma once

#include <memory>
#include <set>

#include "RayPassMesh.hpp"
#include "RayPassNode.hpp"
#include "RayPassMaterial.hpp"
#include "vulkan/DescriptorPool.hpp"
#include "vulkan/DescriptorSet.hpp"
#include "vulkan/Fence.hpp"
#include "vulkan/Image.hpp"
#include "vulkan/Semaphore.hpp"
#include "vulkan/Uniforms.hpp"
#include "vulkan/StaticBuffer.hpp"

#include "types/Node.hpp"

#include "util/IterAdapter.hpp"
#include "util/filter_iterator.hpp"
#include "util/Observer.hpp"

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
			VkImageView image_view();
			VkSemaphore submit(
					Node &node,
					uint32_t count,
					ComputeUniform uniform,
					VkSemaphore semaphore);
			MappedComputeUniform &current_uniform_buffer();

			RayPassMesh &mesh(uint32_t id) { return _meshes[id]; }
			RayPassMesh const &mesh(uint32_t id) const { return _meshes[id]; }

			MeshObserver &mesh_observer() { return _mesh_observer; }
			MaterialObserver &material_observer() { return _material_observer; }
			NodeObserver &node_observer() { return _node_observer; }

			void resize(VkExtent2D size);
			size_t max_material_range() const;
			std::vector<VkImageView> used_textures() const;
			uint32_t compute_index() const { return _compute_index; }
			uint32_t ray_count() const { return _ray_count; }
			void reset_counters();

		private:
			using MeshContainer = std::vector<RayPassMesh>;
			using mesh_iterator = util::filter_iterator<MeshContainer::iterator>;
			using const_mesh_iterator = util::filter_iterator<MeshContainer::const_iterator>;

			using NodeContainer = std::vector<RayPassNode>;
			using node_iterator = util::filter_iterator<NodeContainer::iterator>;
			using const_node_iterator = util::filter_iterator<NodeContainer::const_iterator>;

			using MaterialContainer = std::vector<RayPassMaterial>;
			using material_iterator = util::filter_iterator<MaterialContainer::iterator>;
			using const_material_iterator = util::filter_iterator<MaterialContainer::const_iterator>;

			mesh_iterator _mesh_begin();
			mesh_iterator _mesh_end();
			auto _get_meshes() { return util::Adapt(_mesh_begin(), _mesh_end()); }

			const_mesh_iterator _mesh_begin() const;
			const_mesh_iterator _mesh_end() const;
			auto _get_meshes() const { return util::Adapt(_mesh_begin(), _mesh_end()); }

			node_iterator _node_begin();
			node_iterator _node_end();
			auto _get_nodes() { return util::Adapt(_node_begin(), _node_end()); }

			const_node_iterator _node_begin() const;
			const_node_iterator _node_end() const;
			auto _get_nodes() const { return util::Adapt(_node_begin(), _node_end()); }

			material_iterator _material_begin();
			material_iterator _material_end();
			auto _get_materials() { return util::Adapt(_material_begin(), _material_end()); }

			const_material_iterator _material_begin() const;
			const_material_iterator _material_end() const;
			auto _get_materials() const { return util::Adapt(_material_begin(), _material_end()); }

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

			void _cleanup_images();
			util::Result<void, KError> _create_images();
			void _update_buffers();
			void _create_mesh_buffers();
			void _create_node_buffers();
			void _create_material_buffers();
			std::string _codegen(uint32_t texture_count);


		private:
			VkExtent2D _size;
			Image _result_image;
			Image _accumulator_image;
			Fence _pass_fence;
			Semaphore _semaphore;
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

			bool _vertex_dirty_bit;
			bool _node_dirty_bit;
			bool _material_dirty_bit;
			bool _size_dirty_bit = false;

			StaticBuffer _vertex_buffer;
			StaticBuffer _bvnode_buffer;
			StaticBuffer _node_buffer;
			StaticBuffer _material_buffer;

			std::vector<RayPassMesh> _meshes;
			std::vector<RayPassNode> _nodes;
			std::vector<RayPassMaterial> _materials;

			Scene *_scene;
			uint32_t _compute_index;
			uint32_t _ray_count;
			bool _clear_accumulator;
	};
}
