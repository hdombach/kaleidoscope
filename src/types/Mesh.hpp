#pragma once

#include <vulkan/vulkan.h>

#include "../vulkan/Vertex.hpp"

namespace types {
	class Mesh {
		public:
			using const_iterator = const vulkan::Vertex*;

			virtual ~Mesh() = default;
			virtual const_iterator begin() const = 0;
			virtual const_iterator end() const = 0;
			virtual uint32_t id() const = 0;
			virtual size_t size() const = 0;
	};
}
