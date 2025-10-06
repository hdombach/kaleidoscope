#pragma once

#include <memory>

#include "util/log.hpp"
#include "vulkan/Shader.hpp"
#include "util/result.hpp"
#include "util/KError.hpp"
#include "types/Material.hpp"

namespace vulkan {
	class Scene;
	class PrevPass;

	class PrevPassMaterial {
		public:
			using Ptr = std::unique_ptr<PrevPassMaterial>;

			PrevPassMaterial() = default;

			static util::Result<Ptr, KError> create(
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
			operator bool() const { return exists(); }

			uint32_t id() const;

			/**
			 * @brief Optional pipeline
			 */
			VkPipeline pipeline();
			/**
			 * @brief Optional pipeline layout
			 */
			VkPipelineLayout pipeline_layout();

			const types::Material *material() const {
				log_assert(_material, "material must exist");
				return _material;
			}

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

			util::Result<void, KError> _create();

		private:
			const types::Material *_material;
			PrevPass *_render_pass;
			bool _pipeline_ready;
			VkPipelineLayout _pipeline_layout;
			VkPipeline _pipeline;
	};
}
