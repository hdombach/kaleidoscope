#pragma once

#include <memory>

#include "util/Observer.hpp"
#include "util/result.hpp"
#include "vulkan/DescriptorSet.hpp"
#include "vulkan/Fence.hpp"
#include "vulkan/Semaphore.hpp"
#include "vulkan/Image.hpp"
#include "vulkan/Error.hpp"
#include "vulkan/Pipeline.hpp"
#include "util/UIDList.hpp"

#include "InstancedPassMesh.hpp"
#include "InstancedPassNode.hpp"
#include "vulkan/Uniforms.hpp"

namespace vulkan {
	class Scene;
	/**
	 * @brief Render pass for the instanced preview render pass.
	 */
	class InstancedPass {
		public:
			using Ptr = std::unique_ptr<InstancedPass>;

			/**
			 * @brief Watch when the meshes list is modified in the resource manager
			 */
			class MeshObserver: public util::Observer {
				public:
					MeshObserver() = default;
					MeshObserver(InstancedPass &instanced_pass);
					void obs_create(uint32_t id) override;
					void obs_update(uint32_t id) override;
					void obs_remove(uint32_t id) override;
				private:
					InstancedPass *_instanced_pass;
			};

			/**
			 * @brief Watch when the node list is modified in the resource manager
			 */
			class NodeObserver: public util::Observer {
				public:
					NodeObserver() = default;
					NodeObserver(InstancedPass &instanced_pass);
					void obs_create(uint32_t id) override;
					void obs_update(uint32_t id) override;
					void obs_remove(uint32_t id) override;
				private:
					InstancedPass *_instanced_pass;
			};

		public:

			/*
			 * @brief Create an uninitialized InstancedPass
			 */
			InstancedPass() = default;

			/**
			 * @brief Creates a new Instanced pass
			 * @param[in] size Initial size of the render view
			 */
			static util::Result<Ptr, Error> create(VkExtent2D size, Scene &scene);

			/**
			 * @bief Copy constructor
			 */
			InstancedPass(const InstancedPass &other) = delete;
			/**
			 * @brief Move constructor
			 */
			InstancedPass(InstancedPass &&other);
			/**
			 * @brief Copy assignment
			 */
			InstancedPass& operator=(const InstancedPass &other) = delete;
			/**
			 * @brief Move assignment
			 */
			InstancedPass& operator=(InstancedPass &&other);

			/**
			 * @brief Deconstructs the InstancedPass
			 */
			void destroy();
			~InstancedPass();

			/**
			 * @brief Checks if InstancedPass is initialized
			 */
			bool has_value() const;
			/**
			 * @brief Alias for has_value
			 */
			operator bool() const;

			/**
			 * @brief Submits draw commands to the graphics card
			 * @param[in] semaphore The semaphore to wait on before starting
			 * @returns The semaphore marking when the InstancedPass is done
			 */
			VkSemaphore render(VkSemaphore semaphore, types::Camera const &camera);

			/**
			 * @brief The render pass for the rasterization stage
			 * TODO: might not need to be used
			 */
			RenderPass const &render_pass() const;

			/**
			 * @brief Handle for the preview in imgui
			 */
			VkDescriptorSet imgui_descriptor_set();

			/**
			 * @brief The layout for descriptor set used by the meshes
			 */
			DescriptorSetLayout const &mesh_descriptor_set_layout() const;

			/**
			 * @brief The layout of the attachments used in the main instanced pass pipeline
			 */
			Pipeline::Attachments const &pipeline_attachments() const;

			/**
			 * @brief The primary viewport
			 */
			VkImageView image_view();

			/**
			 * @brief InstancedPass handlers to changes in the mesh list
			 */
			MeshObserver &mesh_observer();

			/**
			 * @brief InstancedPass handlers to changes in the node list
			 */
			NodeObserver &node_observer();

			DescriptorPool const &descriptor_pool() const;

		private:
			util::UIDList<InstancedPassMesh> _meshes;
			util::UIDList<InstancedPassNode> _nodes;
			MeshObserver _mesh_observer;
			NodeObserver _node_observer;
		
			Scene *_scene;
			RenderPass _render_pass;
			Pipeline _pipeline;
			DescriptorPool _descriptor_pool;
			DescriptorSets _shared_descriptor_set;
			Fence _fence;
			Semaphore _semaphore;
			VkCommandBuffer _command_buffer;
			VkExtent2D _size;
			Image _depth_image;
			Image _material_image;
			Image _result_image;
			MappedPrevPassUniform _prim_uniform;
			VkDescriptorSet _imgui_descriptor_set;

		private:
			void mesh_create(uint32_t id);
			void mesh_update(uint32_t id);
			void mesh_remove(uint32_t id);

			void node_create(uint32_t id);
			void node_update(uint32_t id);
			void node_remove(uint32_t id);

		private:
			const static VkFormat _MATERIAL_IMAGE_FORMAT = VK_FORMAT_R16_UINT;
			const static VkFormat _RESULT_IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;

			/**
			 * @brief Sets up the render pass
			 * Does not depend on any other components to be initialized
			 */
			util::Result<RenderPass, Error> _create_render_pass();

			/**
			 * @brief Sets up the pipeline
			 * Requires the descriptor set and render pass to be initialized.
			 */
			util::Result<void, Error>_create_pipeline(Pipeline &pipeline);

			/**
			 * @brief Creates the depth and material images
			 */
			util::Result<void, Error> _create_images();

			/**
			 * @brief Creates the command buffer
			 * Does not depend on any other components to be initialized.
			 */
			util::Result<VkCommandBuffer, Error> _create_command_buffer();

			/**
			 * @brief Figures out the best supported depth format
			 * Does not depend on any other components to be initialized.
			 */
			VkFormat _depth_format();

			/**
			 * @brief Creates the descriptor set
			 * Depends on the images to be initialized.
			 */
			util::Result<void, Error> _create_descriptor_set();

			util::Result<void, Error> _create_uniform();
	};
}
