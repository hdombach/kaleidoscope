#pragma once

#include <vector>
#include <glm/fwd.hpp>
#include <vulkan/vulkan.h>

#include "../util/result.hpp"
#include "../util/errors.hpp"
#include "../types/ShaderResource.hpp"
#include "Shader.hpp"

namespace vulkan {
	class MaterialPreviewImpl;
	class PrevPass;

	class Material {
		public:
			using PreviewImpl = MaterialPreviewImpl;

			virtual ~Material() = default;

			virtual std::vector<types::ShaderResource> const &resources() const = 0;
			virtual uint32_t id() const = 0;
	};
}
