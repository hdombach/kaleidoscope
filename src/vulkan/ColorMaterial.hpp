#pragma once

#include <optional>
#include <vector>

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

#include "DescriptorSet.hpp"
#include "Material.hpp"
#include "PreviewRenderPass.hpp"
#include "MappedUniform.hpp"

namespace vulkan {
	class ColorMaterial: public Material {
		public:
			struct UniformBuffer {
				alignas(16) glm::mat4 object_transformation;
				alignas(16) glm::vec3 color;
			} __attribute__((packed));

			ColorMaterial(uint32_t id, glm::vec3 color);

			~ColorMaterial() override = default;

			std::vector<types::ShaderResource> const &resources() const override;
			uint32_t id() const override;

			ColorMaterial(const ColorMaterial& other) = delete;
			ColorMaterial(ColorMaterial &&other) = default;
			ColorMaterial& operator=(const ColorMaterial& other) = delete;
			ColorMaterial& operator=(ColorMaterial&& other) = default;

		private:
			uint32_t _id;

			glm::vec3 _color;

			std::vector<types::ShaderResource> _resources;

			VType<UniformBuffer> _uniform;
	};
}
