#pragma once

#include <vulkan/vulkan_core.h>

#include "Material.hpp"
#include "../vulkan/Texture.hpp"

namespace types {
	class TextureMaterial: public Material {
		public:
			static std::unique_ptr<TextureMaterial> create(uint32_t id, vulkan::Texture* texture);

			~TextureMaterial() override = default;

			TextureMaterial(const TextureMaterial& other) = delete;
			TextureMaterial(TextureMaterial &&other) = delete;
			TextureMaterial& operator=(const TextureMaterial& other) = delete;
			TextureMaterial& operator=(TextureMaterial&& other) = delete;

			types::ShaderResources const &resources() const override { return _resources; }
			uint32_t id() const override { return _id; }
		private:
			TextureMaterial() = default;
			vulkan::Texture *_texture;
			uint32_t _id;

			ShaderResources _resources;

			glm::mat4 _object_transformation;
			glm::vec3 _default_position;
	};

}
