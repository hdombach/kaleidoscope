#pragma once

#include <string>
#include <memory>

#include "codegen/TemplObj.hpp"
#include "vulkan/Vertex.hpp"

namespace types {
	class Mesh {
		public:
			using Ptr = std::unique_ptr<Mesh>;
			using const_iterator = const vulkan::Vertex*;

			virtual ~Mesh() = default;
			virtual const_iterator begin() const = 0;
			virtual const_iterator end() const = 0;
			virtual uint32_t id() const = 0;
			virtual size_t size() const = 0;
			virtual void set_name(std::string const &name) = 0;
			virtual std::string const &name() const = 0;
			
			virtual bool is_de() const { return false; }
			virtual std::string const &de() const { static std::string s = ""; return s; };
	};
}
