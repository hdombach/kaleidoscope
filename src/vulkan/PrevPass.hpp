#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <vulkan/vulkan_core.h>

#include "DescriptorPool.hpp"
#include "DescriptorSet.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include "Texture.hpp"
#include "UniformBufferObject.hpp"
#include "PrevPassMesh.hpp"
#include "PrevPassMaterial.hpp"
#include "PrevPassNode.hpp"
#include "../util/result.hpp"
#include "../util/errors.hpp"
#include "../util/Observer.hpp"
#include "../types/Camera.hpp"
#include "../types/ShaderResource.hpp"

namespace vulkan {
	class Node;
	class PrevPass: public Texture {
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
			void render(std::vector<Node::Ptr> &nodes, types::Camera &camera);
			void resize(VkExtent2D size) override;
			bool is_resizable() const override;

			VkExtent2D size() const;
			VkDescriptorSet get_descriptor_set() override;
			ImageView const &image_view() override;
			VkRenderPass render_pass();
			MappedGlobalUniform &current_uniform_buffer();
			DescriptorPool &descriptor_pool() { return _descriptor_pool; };
			int frame_index() { return _frame_index; };
			VkDescriptorSetLayout global_descriptor_set_layout() { return _descriptor_sets.layout(); }
			VkDescriptorSet global_descriptor_set(int frame_index) { return _descriptor_sets.descriptor_set(frame_index); }

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

		private:
			PrevPass(Scene &scene, VkExtent2D size);

			util::Result<void, KError> _create_sync_objects();
			void _create_command_buffers();
			util::Result<void, KError> _create_images();
			void _cleanup_images();
			static VkFormat _depth_format();

			std::vector<PrevPassMesh> _meshes;
			std::vector<PrevPassMaterial> _materials;
			std::vector<PrevPassNode> _nodes;
			MeshObserver _mesh_observer;
			MaterialObserver _material_observer;
			NodeObserver _node_observer;

			VkExtent2D _size;
			Image _depth_image;
			ImageView _depth_image_view;
			std::vector<Image> _color_images;
			std::vector<ImageView> _color_image_views;
			std::vector<VkFramebuffer> _framebuffers;
			std::vector<VkDescriptorSet> _imgui_descriptor_sets;
			DescriptorSets _descriptor_sets;
			VkRenderPass _render_pass;
			std::vector<MappedGlobalUniform> _mapped_uniforms;
			std::vector<Fence> _in_flight_fences;
			std::vector<Semaphore> _render_finished_semaphores;
			std::vector<VkCommandBuffer> _command_buffers;
			DescriptorPool _descriptor_pool;
			const static VkFormat _RESULT_IMAGE_FORMAT = VK_FORMAT_R8G8B8A8_SRGB;
			int _frame_index;
			uint32_t _mip_levels;

			Scene *_scene;
	};
}