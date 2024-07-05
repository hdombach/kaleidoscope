#pragma once

#include <string>

#include <glm/glm.hpp>

#include "../vulkan/ImageView.hpp"

namespace types {
	class ShaderResource {
		public:
			enum Type {
				SRUnknown,
				SRImage,
				SRMat4,
				SRVec3,
			};

			static ShaderResource create_primitive(std::string name, glm::mat4 &mat);
			static ShaderResource create_primitive(std::string name, glm::vec3 &vec);

			static ShaderResource create_image(std::string name, vulkan::ImageView const &image_view);

			void const *primitive() const { return _primitive; }
			size_t primitive_size() const { return _primitive_size; }

			Type type() const { return _type; }

			vulkan::ImageView const &image_view() const { return *_image_view; }

			~ShaderResource() = default;

		private:
			ShaderResource(std::string &name, Type type);

			std::string _name;

			void* _primitive;
			size_t _primitive_size;

			const vulkan::ImageView *_image_view;
			Type _type;
	};
}
