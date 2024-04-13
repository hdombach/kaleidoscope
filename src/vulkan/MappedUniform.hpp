#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "graphics.hpp"
#include "../util/result.hpp"

namespace vulkan {
	template<typename T>
	class MappedUniform {
		public:
			using BufferObj = T;

			MappedUniform():
				_buffer(nullptr), _buffer_memory(nullptr), _uniform_buffer_mapped(nullptr)
			{}

			static util::Result<MappedUniform<T>, KError> create() {
				auto result = MappedUniform();
				auto buffer_size = VkDeviceSize(sizeof(T));

				Graphics::DEFAULT->createBuffer(
						buffer_size, 
						VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
						result._buffer, 
						result._buffer_memory);

				auto res = vkMapMemory(
						Graphics::DEFAULT->device(), 
						result._buffer_memory, 
						0, 
						buffer_size, 
						0, 
						(void **) &result._uniform_buffer_mapped);

				if (res == VK_SUCCESS) {
					return {std::move(result)};
				} else {
					return {res};
				}
			}

			MappedUniform(const MappedUniform& other) = delete;
			MappedUniform(MappedUniform &&other) {
				_buffer = other._buffer;
				_buffer_memory = other._buffer_memory;
				_uniform_buffer_mapped = other._uniform_buffer_mapped;

				other._buffer = nullptr;
				other._buffer_memory = nullptr;
				other._uniform_buffer_mapped = nullptr;
			}

			MappedUniform& operator=(const MappedUniform& other) = delete;
			MappedUniform& operator=(MappedUniform &&other) {
				_buffer = other._buffer;
				_buffer_memory = other._buffer_memory;
				_uniform_buffer_mapped = other._uniform_buffer_mapped;

				other._buffer = nullptr;
				other._buffer_memory = nullptr;
				other._uniform_buffer_mapped = nullptr;
				
				return *this;
			}

			VkBuffer& buffer() {
				return _buffer;
			}
			VkBuffer const& buffer() const {
				return _buffer;
			}

			~MappedUniform() {
				if (_buffer) {
					vkDestroyBuffer(Graphics::DEFAULT->device(), _buffer, nullptr);
					_buffer = nullptr;
				}

				if (_buffer_memory) {
					vkFreeMemory(Graphics::DEFAULT->device(), _buffer_memory, nullptr);
					_buffer_memory = nullptr;
					_uniform_buffer_mapped = nullptr;
				}
			}

			bool has_value() {
				return _uniform_buffer_mapped != nullptr;
			}
			T value() const {
				return *_uniform_buffer_mapped;
			}
			void set_value(T const &buffer) {
				*_uniform_buffer_mapped = buffer;
			}

		private:

			VkBuffer _buffer;
			VkDeviceMemory _buffer_memory;
			T *_uniform_buffer_mapped;
	};
}
