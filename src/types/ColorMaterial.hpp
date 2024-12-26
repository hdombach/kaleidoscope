#pragma once

#include <memory>
#include <string>
#include <cstdint>

#include <glm/fwd.hpp>

#include "Material.hpp"
#include "types/ShaderResource.hpp"

namespace types {
	class ColorMaterial: public Material {
		public:
			static std::unique_ptr<ColorMaterial> create(uint32_t id, glm::vec3 color);

			~ColorMaterial() override = default;

			ShaderResources const &resources() const override { return _resources; }
			uint32_t id() const override { return _id; }
			std::string const &frag_shader_src() const override { return _frag_shader_src; }

			ColorMaterial(const ColorMaterial& other) = delete;
			ColorMaterial(ColorMaterial &&other) = delete;
			ColorMaterial& operator=(const ColorMaterial& other) = delete;
			ColorMaterial& operator=(ColorMaterial&& other) = delete;

		private:
			ColorMaterial() = default;

			uint32_t _id;
			std::string _frag_shader_src;

			ShaderResources _resources;

			glm::vec3 _color;
			glm::vec3 _default_position;
	};
}
