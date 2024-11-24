#pragma once

#include "vulkan/Shader.hpp"
#include "util/result.hpp"
#include "util/errors.hpp"
#include "types/Material.hpp"

namespace vulkan {
	class Scene;
	class PrevPass;

	class PrevPassMaterial {
		public:
			PrevPassMaterial() = default;

			static util::Result<PrevPassMaterial, KError> create(
					Scene &scene,
					PrevPass &preview_pass,
					const types::Material *material);

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

		private:
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

			static void _codegen(
					std::string &frag_source,
					std::string &vert_source,
					const types::Material *material,
					std::vector<std::string> &textures);

		private:
			const types::Material *_material;
			PrevPass *_render_pass;
			VkPipelineLayout _pipeline_layout;
			VkPipeline _pipeline;

	};
}
