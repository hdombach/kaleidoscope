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
#include "vulkan/Fence.hpp"
#include "vulkan/Semaphore.hpp"
#include "util/result.hpp"
#include "util/KError.hpp"
#include "util/Observer.hpp"
#include "util/UIDList.hpp"
#include "types/Camera.hpp"

namespace vulkan {
	class Node;
	class PrevPass {
		public:
			enum class ErrorType {
				VULKAN,
				RESOURCE,
			};

			using Error = TypedError<ErrorType>;

			using Ptr = std::unique_ptr<PrevPass>;

			/**
			 * @brief Watch when meshes list is modified in resource manager
			 */
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

			/**
			 * @brief Watch when material list is modified in resource manager
			 */
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

			/**
			 * @brief Watch when node list is modified in resource manager
			 */
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
			/**
			 * @brief Handle for preview in imgui
			 */
			VkDescriptorSet imgui_descriptor_set();
			/**
			 * @brief Primary viepwort
			 */
			VkImageView image_view();
			/**
			 * @brief Render pass of primary rasterization stage
			 * Needs to be used by PrevPassMaterial to use in pipelines
			 * The DE stage is handeled in a seperate render pass internally
			 */
			VkRenderPass render_pass();
			DescriptorPool &descriptor_pool() { return _descriptor_pool; };
			/**
			 * @brief Descriptor set layout used across rasterization and de render pass
			 */
			VkDescriptorSetLayout shared_descriptor_set_layout() { return _shared_descriptor_set_layout.layout(); }
			/**
			 * @brief Descriptor set used across rasterization and de render pass
			 */
			VkDescriptorSet shared_descriptor_set() { return _shared_descriptor_set.descriptor_set(0); }

			/**
			 * @brief PrevPass handlers to changes in materials list
			 */
			MaterialObserver &material_observer() { return _material_observer; }
			/**
			 * @brief PrevPass handlers to changes in mesh list
			 */
			MeshObserver &mesh_observer() { return _mesh_observer; }
			/**
			 * @brief PrevPass handlers to changes in node list
			 */
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

			util::Result<void, KError> _create_overlay_descriptor_set();

			util::Result<void, KError> _create_overlay_pipeline();
			void _destroy_overlay_pipeline();

			util::Result<void, KError> _create_de_descriptor_set();
			void _destroy_de_descriptor_set();

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

			std::string _codegen_de();

		private:
			util::UIDList<PrevPassMesh> _meshes;
			util::UIDList<PrevPassMaterial::Ptr, util::has_value, util::id_deref_trait> _materials;
			util::UIDList<PrevPassNode> _nodes;
			MeshObserver _mesh_observer;
			MaterialObserver _material_observer;
			NodeObserver _node_observer;

			VkExtent2D _size;
			Image _depth_image;
			Image _depth_buf_image;
			Image _color_image;
			/** @brief Texture written to by rasterizer and read from by de pass*/
			Image _node_image;
			/** @brief Texture written to by de pass (accumulates _node_image) */
			Image _de_node_image;

			DescriptorSetLayout _shared_descriptor_set_layout;
			DescriptorSets _shared_descriptor_set;

			VkRenderPass _prim_render_pass;
			VkFramebuffer _prim_framebuffer;
			MappedPrevPassUniform _prim_uniform;

			VkPipelineLayout _de_pipeline_layout;
			VkPipeline _de_pipeline;
			VkRenderPass _de_render_pass;
			VkFramebuffer _de_framebuffer;
			DescriptorSetLayout _de_descriptor_set_layout;
			DescriptorSets _de_descriptor_set;
			StaticBuffer _de_node_buffer;
			StaticBuffer _de_material_buffer;

			MappedOverlayUniform _mapped_overlay_uniform;
			VkPipelineLayout _overlay_pipeline_layout;
			VkPipeline _overlay_pipeline;
			DescriptorSetLayout _overlay_descriptor_set_layout;
			DescriptorSets _overlay_descriptor_set;

			VkDescriptorSet _imgui_descriptor_set;
			Fence _fence;
			Semaphore _semaphore;
			VkCommandBuffer _command_buffer;
			DescriptorPool _descriptor_pool;

			bool _de_buf_dirty_bit;
			bool _de_pipe_dirty_bit;

			const static VkFormat _RESULT_IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
			const static VkFormat _NODE_IMAGE_FORMAT = VK_FORMAT_R16_UINT;
			const static VkFormat _DEPTH_BUF_IMAGE_FORMAT = VK_FORMAT_R8_SRGB;

			Scene *_scene;
	};
}

template<>
	const char *vulkan::PrevPass::Error::type_str(vulkan::PrevPass::ErrorType t);

std::ostream &operator<<(std::ostream &os, vulkan::PrevPass::ErrorType err);
