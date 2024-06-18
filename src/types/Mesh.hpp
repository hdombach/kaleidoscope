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

			virtual VkBuffer vertex_buffer() const = 0;
			virtual VkBuffer index_buffer() const = 0;
			virtual uint32_t index_count() const = 0;
			virtual VkDeviceSize vertex_buffer_range() const = 0;
			virtual VkDeviceSize index_buffer_range() const = 0;
	};
}
