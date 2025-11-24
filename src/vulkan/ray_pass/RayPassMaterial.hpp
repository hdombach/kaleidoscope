#pragma once

#include <string>
#include "util/BaseError.hpp"
#include "util/result.hpp"
#include "types/Material.hpp"

namespace vulkan {
	class RayPass;
	class Node;

	class RayPassMaterial {
		public:
			enum class ErrorType {
				MISC,
				INVALID_ARG,
			};

			using Error = TypedError<ErrorType>;

			RayPassMaterial();
			static util::Result<RayPassMaterial, RayPassMaterial::Error> create(
					const types::Material *material,
					const RayPass *ray_pass);

			bool has_value() const { return _material; }
			operator bool() const { return has_value(); }
			uint32_t id() const { return _material->id(); }

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
	};
}

template<>
const char *vulkan::RayPassMaterial::Error::type_str(vulkan::RayPassMaterial::ErrorType t);

std::ostream &operator<<(std::ostream &os, vulkan::RayPassMaterial::ErrorType t);
