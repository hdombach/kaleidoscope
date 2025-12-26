#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "util/result.hpp"
#include "Error.hpp"
#include "Vertex.hpp"

namespace vulkan {
	class StaticBuffer {
		public:
			static util::Result<StaticBuffer, Error> create(
				void *data,
				VkDeviceSize range,
				VkBufferUsageFlags usage
			);

			template<typename T>
				static util::Result<StaticBuffer, Error> create(std::vector<T> &buf) {
					return create(
						buf.data(),
						buf.size() * sizeof(T),
						VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
					);
				}

			static util::Result<StaticBuffer, Error> create_vertices(std::vector<Vertex> &buf) {
				return create(
					buf.data(),
					buf.size() * sizeof(Vertex),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
				);
			}

			static util::Result<StaticBuffer, Error> create_indices(std::vector<uint32_t> &buf) {
				return create(
					buf.data(),
					buf.size() * sizeof(uint32_t),
					VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
				);
			}

			StaticBuffer(const StaticBuffer& other) = delete;
			StaticBuffer(StaticBuffer &&other);
			StaticBuffer& operator=(const StaticBuffer& other) = delete;
			StaticBuffer& operator=(StaticBuffer&& other);

			StaticBuffer();

			void destroy();
			~StaticBuffer();

			VkBuffer buffer() { return _buffer; }
			const VkBuffer buffer() const { return _buffer; }
			VkDeviceSize range() const { return _range; }

			operator bool() const {
				return _buffer != nullptr;
			}

		private:
			VkBuffer _buffer;
			VkDeviceMemory _buffer_memory;
			VkDeviceSize _range;
	};
}
