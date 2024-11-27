#pragma once

#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "PrevPassMesh.hpp"
#include "PrevPassMaterial.hpp"
#include "PrevPassNode.hpp"
#include "vulkan/DescriptorPool.hpp"
#include "vulkan/DescriptorSet.hpp"
#include "vulkan/Image.hpp"
#include "vulkan/Uniforms.hpp"
#include "util/result.hpp"
#include "util/errors.hpp"
#include "util/Observer.hpp"
#include "types/Camera.hpp"

namespace vulkan {
	class Node;
	class PrevPass {
		public:
			using Ptr = std::unique_ptr<PrevPass>;

			class MeshObserver: public util::Observer {
				public:
					MeshObserver() = default;
					MeshObserver(PrevPass &render_pass);
					void obs_create(uint32_t id) override;
					void obs_update(uint32_t id) override;
					void obs_remove(uint32_t id) override;
				private:
					PrevPass *_render_pass;
			};

			class MaterialObserver: public util::Observer {
				public:
					MaterialObserver() = default;
					MaterialObserver(PrevPass &render_pass);
					void obs_create(uint32_t id) override;
					void obs_update(uint32_t id) override;
					void obs_remove(uint32_t id) override;
				private:
					PrevPass *_render_pass;
			};

			class NodeObserver: public util::Observer {
				public:
					NodeObserver() = default;
					NodeObserver(PrevPass &render_pass);
					void obs_create(uint32_t id) override;
					void obs_update(uint32_t id) override;
					void obs_remove(uint32_t id) override;
				private:
					PrevPass *_render_pass;
			};

			static util::Result<Ptr, KError> create(
					Scene &scene,
					VkExtent2D size);
			void destroy();
			~PrevPass();
			VkSemaphore render(
					std::vector<Node::Ptr> &nodes,
					types::Camera const &camera,
					VkSemaphore semaphore);
			void resize(VkExtent2D size);

			VkExtent2D size() const;
			VkDescriptorSet imgui_descriptor_set();
			VkImageView image_view();
			VkRenderPass render_pass();
			DescriptorPool &descriptor_pool() { return _descriptor_pool; };
			VkDescriptorSetLayout shared_descriptor_set_layout() { return _shared_descriptor_set.layout(); }
			VkDescriptorSet shared_descriptor_set() { return _shared_descriptor_set.descriptor_set(0); }

			MaterialObserver &material_observer() { return _material_observer; }
			MeshObserver &mesh_observer() { return _mesh_observer; }
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

			PrevPass(Scene &scene, VkExtent2D size);

			util::Result<void, KError> _create_prim_render_pass();
			void _destroy_prim_render_pass();

			util::Result<void, KError> _create_overlay_pipeline();
			void _destroy_overlay_pipeline();

			util::Result<void, KError> _create_de_render_pass();
			void _destroy_de_render_pass();

			util::Result<void, KError> _create_de_pipeline();
			void _destroy_de_pipeline(); 

			util::Result<void, KError> _create_de_buffers();
			void _destroy_de_buffers();

			util::Result<void, KError> _create_images();
			void _cleanup_images();

			util::Result<void, KError> _create_shared_descriptor_set();
			void _destroy_shared_descriptor_set();

			util::Result<void, KError> _create_framebuffers();
			void _destroy_framebuffers();

			util::Result<void, KError> _create_sync_objects();
			void _create_command_buffers();
			static VkFormat _depth_format();

			static std::string _codegen_de();

		private:
			std::vector<PrevPassMesh> _meshes;
			std::vector<PrevPassMaterial> _materials;
			std::vector<PrevPassNode> _nodes;
			MeshObserver _mesh_observer;
			MaterialObserver _material_observer;
			NodeObserver _node_observer;

			VkExtent2D _size;
			Image _depth_image;
			Image _depth_buf_image;
			Image _color_image;
			Image _node_image;

			DescriptorSets _shared_descriptor_set;

			VkRenderPass _prim_render_pass;
			VkFramebuffer _prim_framebuffer;
			MappedPrevPassUniform _prim_uniform;

			VkPipelineLayout _de_pipeline_layout;
			VkPipeline _de_pipeline;
			VkRenderPass _de_render_pass;
			VkFramebuffer _de_framebuffer;
			DescriptorSets _de_descriptor_set;
			StaticBuffer _de_node_buffer;

			MappedOverlayUniform _mapped_overlay_uniform;
			VkPipelineLayout _overlay_pipeline_layout;
			VkPipeline _overlay_pipeline;
			DescriptorSets _overlay_descriptor_set;

			VkDescriptorSet _imgui_descriptor_set;
			Fence _fence;
			Semaphore _semaphore;
			VkCommandBuffer _command_buffer;
			DescriptorPool _descriptor_pool;

			const static VkFormat _RESULT_IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
			const static VkFormat _NODE_IMAGE_FORMAT = VK_FORMAT_R16_UINT;
			const static VkFormat _DEPTH_BUF_IMAGE_FORMAT = VK_FORMAT_R8_SRGB;

			Scene *_scene;
	};
}
