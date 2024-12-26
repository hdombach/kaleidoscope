#pragma once

#include <cstdint>
#include <memory>
#include <glm/fwd.hpp>

#include "Material.hpp"
#include "ShaderResource.hpp"
#include "vulkan/Texture.hpp"

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
			std::string const &frag_shader_src() const override { return _frag_shader_src; }
		private:
			TextureMaterial() = default;
			vulkan::Texture *_texture;
			uint32_t _id;
			std::string _frag_shader_src;

			ShaderResources _resources;

			glm::vec3 _default_position;
	};

}
