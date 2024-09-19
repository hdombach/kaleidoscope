#pragma once

#include <memory>

#include "Material.hpp"
#include "../vulkan/Texture.hpp"

namespace types {
	class CombTextureMaterial: public Material {
		public:
			static std::unique_ptr<CombTextureMaterial> create(
					uint32_t id,
					vulkan::Texture* prim_texture,
					vulkan::Texture* comb_texture);

			~CombTextureMaterial() override = default;

			CombTextureMaterial(const CombTextureMaterial& other) = delete;
			CombTextureMaterial(CombTextureMaterial &&other) = delete;
			CombTextureMaterial& operator=(const CombTextureMaterial& other) = delete;
			CombTextureMaterial& operator=(CombTextureMaterial &&other) = delete;

			types::ShaderResources const &resources() const override { return _resources; }
			uint32_t id() const override { return _id; }
			std::string const &frag_shader_src() const override { return _frag_shader_src; }

			float comb_ratio() const;
			void set_comb_ratio(float ratio) {
				_resources.set_float("comb_ratio", ratio);
			}
		private:
			CombTextureMaterial() = default;
			vulkan::Texture *_prim_texture;
			vulkan::Texture *_comb_texture;
			uint32_t _id;
			std::string _frag_shader_src;

			ShaderResources _resources;
	};
}
