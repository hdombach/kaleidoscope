#pragma once

#include <glm/fwd.hpp>

#include "../types/ShaderResource.hpp"

namespace types {

	class Material {
		public:
			virtual ~Material() = default;

			virtual types::ShaderResources const &resources() const = 0;
			virtual uint32_t id() const = 0;
	};
}
