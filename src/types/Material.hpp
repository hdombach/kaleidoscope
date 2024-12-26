#pragma once

#include <string>

#include "types/ShaderResource.hpp"

namespace types {

	class Material {
		public:
			virtual ~Material() = default;

			virtual types::ShaderResources const &resources() const = 0;
			virtual uint32_t id() const = 0;
			virtual std::string const &frag_shader_src() const = 0;

			void set_name(std::string const &name) { _name = name; }
			std::string const &name() const { return _name; }

		protected:
			std::string _name;
	};
}
