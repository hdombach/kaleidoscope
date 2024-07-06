#pragma once

#include "../util/result.hpp"
#include "../util/errors.hpp"
#include "../vulkan/Material.hpp"
#include "../vulkan/DescriptorSet.hpp"

namespace vulkan {
	class Scene;

	class PrevPassMaterial {
		public:
			PrevPassMaterial() = default;

			static util::Result<PrevPassMaterial, KError> create(
					Scene &scene,
					PrevPass &preview_pass,
					const vulkan::Material *material);

			PrevPassMaterial(const PrevPassMaterial& other) = delete;
			PrevPassMaterial(PrevPassMaterial &&other);
			PrevPassMaterial& operator=(const PrevPassMaterial& other) = delete;
			PrevPassMaterial& operator=(PrevPassMaterial&& other);

			void destroy();

			~PrevPassMaterial();

			bool exists() const;
			operator bool() { return exists(); }

			uint32_t id() const;

			VkPipeline pipeline() { return _pipeline; }
			VkPipelineLayout pipeline_layout() { return _pipeline_layout; }
			VkDescriptorSet get_descriptor_set() { return _descriptor_sets.descriptor_set(0); }

		private:
			const vulkan::Material *_material;
			Uniform _global_uniform;
			DescriptorSets _descriptor_sets;
			PrevPass *_render_pass;
			VkPipelineLayout _pipeline_layout;
			VkPipeline _pipeline;

			/**
			 * @param[in] vertex_shader
			 * @param[in] fragment_shader
			 * @param[in] descriptor_set_layout
			 * @param[out] pipeline
			 * @param[out] pipeline_layout
			 */
			static util::Result<void, KError> _create_pipeline(
					Shader &vertex_shader,
					Shader &fragment_shader,
					PrevPass &render_pass,
					std::vector<VkDescriptorSetLayout> &descriptor_set_layouts,
					VkPipeline *pipeline,
					VkPipelineLayout *pipeline_layout);

	};
}
