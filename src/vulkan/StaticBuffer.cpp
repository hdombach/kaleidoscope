#include "StaticBuffer.hpp"

#include <vulkan/vulkan_core.h>

#include "util/result.hpp"
#include "graphics.hpp"

namespace vulkan {
	util::Result<StaticBuffer, Error> StaticBuffer::create(
			void *data,
			VkDeviceSize range)
	{
		auto result = StaticBuffer();

		result._range = range;

		VkBuffer staging_buffer;
		VkDeviceMemory staging_buffer_memory;
		
		if (range == 0) {
			return Error(ErrorType::EMPTY_BUFFER, "Cannot create empty static buffer");
		}

		Graphics::DEFAULT->create_buffer(
				range, 
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
				staging_buffer, 
				staging_buffer_memory);

		void *mapped_data;
		auto res = vkMapMemory(
				Graphics::DEFAULT->device(), 
				staging_buffer_memory, 
				0, 
				range,
				0,
				&mapped_data);
		if (res != VK_SUCCESS) {
			return Error(ErrorType::VULKAN, "Cannot map memory", {res});
		}

		memcpy(mapped_data, data, range);

		vkUnmapMemory(Graphics::DEFAULT->device(), staging_buffer_memory);

		Graphics::DEFAULT->create_buffer(
				range, 
				VK_BUFFER_USAGE_TRANSFER_DST_BIT
				| VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
				| VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				result._buffer,
				result._buffer_memory);

		Graphics::DEFAULT->copy_buffer(staging_buffer, result._buffer, range);

		vkDestroyBuffer(Graphics::DEFAULT->device(), staging_buffer, nullptr);
		vkFreeMemory(Graphics::DEFAULT->device(), staging_buffer_memory, nullptr);

		return {std::move(result)};
	}

	StaticBuffer::StaticBuffer(StaticBuffer &&other) {
		_buffer = other._buffer;
		other._buffer = nullptr;

		_buffer_memory = other._buffer_memory;
		other._buffer_memory = nullptr;

		_range = other._range;
	}

	StaticBuffer& StaticBuffer::operator=(StaticBuffer&& other) {
		destroy();

		_buffer = other._buffer;
		other._buffer = nullptr;

		_buffer_memory = other._buffer_memory;
		other._buffer_memory = nullptr;

		_range = other._range;

		return *this;
	}

	StaticBuffer::StaticBuffer():
		_buffer(nullptr),
		_buffer_memory(nullptr),
		_range(0)
	{}

	void StaticBuffer::destroy() {
		if (_buffer) {
			vkDestroyBuffer(Graphics::DEFAULT->device(), _buffer, nullptr);
			_buffer = nullptr;
		}
		if (_buffer_memory) {
			vkFreeMemory(Graphics::DEFAULT->device(), _buffer_memory, nullptr);
			_buffer_memory = nullptr;
		}
	}

	StaticBuffer::~StaticBuffer() {
		destroy();
	}
}
