#pragma once

#include <vector>

#include <vulkan/vulkan_core.h>

#include "Material.hpp"
#include "Texture.hpp"
#include "VType.hpp"

namespace vulkan {
	class TextureMaterial: public Material {
		public:
			struct UniformBuffer {
				alignas(16) glm::mat4 object_transformation;
			};

			TextureMaterial(uint32_t id, Texture* texture);

			~TextureMaterial() override = default;

			TextureMaterial(const TextureMaterial& other) = delete;
			TextureMaterial(TextureMaterial &&other) = default;
			TextureMaterial& operator=(const TextureMaterial& other) = delete;
			TextureMaterial& operator=(TextureMaterial&& other) = default;

			std::vector<types::ShaderResource> const &resources() const override { return _resources; }
			uint32_t id() const override { return _id; }
		private:
			Texture *_texture;
			uint32_t _id;

			std::vector<types::ShaderResource> _resources;

			VType<UniformBuffer> _uniform;
	};

}
