#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "util/result.hpp"
#include "util/KError.hpp"

namespace vulkan {
	class StaticBuffer {
		public:
			static util::Result<StaticBuffer, KError> create(void *data, VkDeviceSize range);

			template<typename T>
				static util::Result<StaticBuffer, KError> create(std::vector<T> &buf) {
					return create(buf.data(), buf.size() * sizeof(T));
				}

			StaticBuffer(const StaticBuffer& other) = delete;
			StaticBuffer(StaticBuffer &&other);
			StaticBuffer& operator=(const StaticBuffer& other) = delete;
			StaticBuffer& operator=(StaticBuffer&& other);

			StaticBuffer();

			void destroy();
			~StaticBuffer();

			VkBuffer buffer() { return _buffer; }
			VkDeviceSize range() { return _range; }

			operator bool() const {
				return _buffer != nullptr;
			}

		private:
			VkBuffer _buffer;
			VkDeviceMemory _buffer_memory;
			VkDeviceSize _range;
	};
}
