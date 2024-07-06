#pragma once

#include <vector>

#include <vulkan/vulkan_core.h>

#include "Material.hpp"
#include "Texture.hpp"

namespace vulkan {
	class TextureMaterial: public Material {
		public:
			static std::unique_ptr<TextureMaterial> create(uint32_t id, Texture* texture);

			~TextureMaterial() override = default;

			TextureMaterial(const TextureMaterial& other) = delete;
			TextureMaterial(TextureMaterial &&other) = delete;
			TextureMaterial& operator=(const TextureMaterial& other) = delete;
			TextureMaterial& operator=(TextureMaterial&& other) = delete;

			std::vector<types::ShaderResource> const &resources() const override { return _resources; }
			uint32_t id() const override { return _id; }
		private:
			TextureMaterial() = default;
			Texture *_texture;
			uint32_t _id;

			std::vector<types::ShaderResource> _resources;

			glm::mat4 _object_transformation;
	};

}
