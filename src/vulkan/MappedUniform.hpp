#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "graphics.hpp"
#include "util/result.hpp"
#include "Error.hpp"

namespace vulkan {
	class Uniform {
		public:
			Uniform():
				_buffer(nullptr),
				_buffer_memory(nullptr),
				_uniform_buffer_mapped(nullptr),
				_buffer_s(0)
			{}

			static util::Result<Uniform, Error> create(size_t buffer_s) {
				auto result = Uniform();
				
				result._buffer_s = buffer_s;

				Graphics::DEFAULT->create_buffer(
						result._buffer_s, 
						VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
						VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
						result._buffer, 
						result._buffer_memory);

				auto res = vkMapMemory(
						Graphics::DEFAULT->device(), 
						result._buffer_memory, 
						0, 
						result._buffer_s, 
						0, 
						(void **) &result._uniform_buffer_mapped);

				if (res == VK_SUCCESS) {
					return {std::move(result)};
				} else {
					return Error(ErrorType::VULKAN, "Could not map memory", {res});
				}
			}

			Uniform(const Uniform& other) = delete;
			Uniform(Uniform &&other) {
				_buffer = other._buffer;
				_buffer_memory = other._buffer_memory;
				_uniform_buffer_mapped = other._uniform_buffer_mapped;
				_buffer_s = other._buffer_s;

				other._buffer = nullptr;
				other._buffer_memory = nullptr;
				other._uniform_buffer_mapped = nullptr;
			}
			Uniform& operator=(const Uniform& other) = delete;
			Uniform& operator=(Uniform &&other) {
				destroy();

				_buffer = other._buffer;
				_buffer_memory = other._buffer_memory;
				_uniform_buffer_mapped = other._uniform_buffer_mapped;
				_buffer_s = other._buffer_s;

				other._buffer = nullptr;
				other._buffer_memory = nullptr;
				other._uniform_buffer_mapped = nullptr;

				return *this;
			}

			void destroy() {
				if (_buffer) {
					Graphics::DEFAULT->wait_idle();
					vkDestroyBuffer(Graphics::DEFAULT->device(), _buffer, nullptr);
					_buffer = nullptr;
				}

				if (_buffer_memory) {
					vkFreeMemory(Graphics::DEFAULT->device(), _buffer_memory, nullptr);
					_buffer_memory = nullptr;
					_uniform_buffer_mapped = nullptr;
				}
			}

			~Uniform() {
				destroy();
			}

			VkBuffer& buffer() {
				return _buffer;
			}
			VkBuffer const& buffer() const {
				return _buffer;
			}
			bool has_value() {
				return _uniform_buffer_mapped != nullptr;
			}
			void const *raw_value() const {
				return _uniform_buffer_mapped;
			}
			void *raw_value() {
				return _uniform_buffer_mapped;
			}
			size_t size() {
				return _buffer_s;
			}

		protected:
			size_t _buffer_s;
			VkBuffer _buffer;
			VkDeviceMemory _buffer_memory;
			void *_uniform_buffer_mapped;
	};

	template<typename T>
	class MappedUniform: public Uniform {
		public:
			using BufferObj = T;

			MappedUniform(): Uniform() {}

			MappedUniform(Uniform &&uniform):
				Uniform(std::move(uniform))
			{ }

			static util::Result<MappedUniform<T>, Error> create() {
				if (auto res = Uniform::create(sizeof(T))) 
					return {std::move(res.value())};
				else
					return res.error();
			}

			VkBuffer& buffer() {
				return _buffer;
			}
			VkBuffer const& buffer() const {
				return _buffer;
			}

			T value() const {
				return *(T *) _uniform_buffer_mapped;
			}
			void set_value(T const &buffer) {
				*(T *) _uniform_buffer_mapped = buffer;
			}
	};
}
