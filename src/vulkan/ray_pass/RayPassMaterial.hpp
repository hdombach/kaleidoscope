#pragma once

#include <string>
#include "util/result.hpp"
#include "util/KError.hpp"
#include "types/Material.hpp"
#include "codegen/TemplObj.hpp"

namespace vulkan {
	class RayPass;
	class Node;

	class RayPassMaterial {
		public:
			RayPassMaterial();
			static util::Result<RayPassMaterial, KError> create(
					const types::Material *material,
					const RayPass *ray_pass);

			bool has_value() const { return _material; }
			operator bool() const { return has_value(); }
			uint32_t id() const { return _material->id(); }

			/**
			 * @brief Creates definition for struct containing properties in vulkan shader
			 */
			std::string const &cg_struct_decl();
			/**
			 * @brief Creates buffer decl using material struct in vulkan shader
			 */
			std::string const &cg_buf_decl();
			/**
			 * @brief Defines function for calculating material color in vulkan shader
			 */
			std::string const &cg_frag_def();
			/**
			 * @brief Code for calling frag function using material buffer in vulkan shader
			 */
			std::string const &cg_frag_call();
			/**
			 * Get templobj description for codegen
			 */
			cg::TemplObj const &cg_templobj();
			/**
			 * @brief Gets underlying generic material
			 */
			const types::Material *get() const { return _material; }
			void update();

		private:
			const types::Material *_material;
			const RayPass *_ray_pass;

			std::string _cg_struct_decl;
			std::string _cg_buf_decl;
			std::string _cg_frag_def;
			std::string _cg_frag_call;
			cg::TemplObj _cg_templobj;

		private:
			void _create_struct_decl();
			void _create_buf_decl();
			void _create_frag_def();
			void _create_frag_call();
			void _create_cg_templobj();
	};
}
