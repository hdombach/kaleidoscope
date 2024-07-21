#pragma once

#include <array>
#include <cstddef>

#include "vulkan/vulkan_core.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>

namespace vulkan {
	struct Vertex {
		alignas(16) glm::vec3 pos;
		alignas(16) glm::vec3 color;
		alignas(8) glm::vec2 tex_coord;

		Vertex() = default;

		Vertex(float x, float y, float z):
			pos(x, y, z),
			color(),
			tex_coord()
		{}

		Vertex(glm::vec3 pos, glm::vec3 color, glm::vec2 tex_coord):
			pos(pos),
			color(color),
			tex_coord(tex_coord)
		{}

		bool operator==(const Vertex& other) const {
			return pos == other.pos && color == other.color && tex_coord == other.tex_coord;
		}

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription binding_description{};
			binding_description.binding = 0;
			binding_description.stride = sizeof(Vertex);
			binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return binding_description;
		}

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};

			attribute_descriptions[0].binding = 0;
			attribute_descriptions[0].location = 0;
			attribute_descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attribute_descriptions[0].offset = offsetof(Vertex, pos);

			attribute_descriptions[1].binding = 0;
			attribute_descriptions[1].location = 1;
			attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attribute_descriptions[1].offset = offsetof(Vertex, color);

			attribute_descriptions[2].binding = 0;
			attribute_descriptions[2].location = 2;
			attribute_descriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attribute_descriptions[2].offset = offsetof(Vertex, tex_coord);

			return attribute_descriptions;
		}
	} __attribute__((packed));

}

namespace std {
	template<> struct hash<vulkan::Vertex> {
		size_t operator()(vulkan::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.tex_coord) << 1);
		}
	};
}
