#pragma once

#include <glm/glm.hpp>
#include <string>

#include "../util/format.hpp"

namespace vulkan {
	struct ObjectUniformBuffer {
		glm::mat4 object_transformation;
	};

	struct ColorBufferObject {
		glm::mat4 object_transformation;
		glm::vec3 color;
	};

	/**
	 * @brief A type that can be accessed through both vulkan an c++
 */
	template<typename T>
	class VType {
		public:
			std::string declaration(std::string &name) {
				return declarator() + " " + name + ";";
			}

			std::string declarator() {
				return "UNKNOWN";
			}

			T& operator*() { return _value; }

			T* get() {
				static_assert(sizeof(T) == sizeof(*this), "VType should be as big as the underlying type");
				return &_value;
			}
			
			size_t size() { return sizeof(T); }

		private:
			T _value;
	} __attribute__((packed));

	template<>
		inline std::string VType<glm::vec4>::declarator() {
			return "vec4";
		}

	template<>
		inline std::string VType<glm::vec3>::declarator() {
			return "vec3";
		}

	template<>
		inline std::string VType<glm::vec2>::declarator() {
			return "vec2";
		}

};
