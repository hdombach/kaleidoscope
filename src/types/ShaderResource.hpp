#pragma once

#include <vector>
#include <string>

#include "../vulkan/VType.hpp"
#include "../vulkan/ImageView.hpp"

namespace types {
	class ShaderResource {
		public:
			enum Type {
				UniformT,
				ImageT,
				ImageTargetT,
				StorageBufferT,
			};

			template<typename T>
				static ShaderResource create_uniform(std::string name, vulkan::VType<T> &type) {
					auto result = ShaderResource(name);
					result._objects = type.get();
					result._object_size = sizeof(T);
					result._object_count = 1;
					return result;
				}

			static ShaderResource create_image(std::string name, vulkan::ImageView const &image_view);

			static ShaderResource create_image_target(std::string name, vulkan::ImageView const &image_view);

			template<typename T>
				static ShaderResource create_storage_buffer(std::string name, std::vector<vulkan::VType<T>> &buffer);

			size_t range() const;
			void const *objects() const;
			size_t object_size() const;
			size_t object_count() const;

			Type type() const;

			vulkan::ImageView const &image_view() const;

			~ShaderResource();

		private:
			ShaderResource(std::string &name);

			std::string _name;

			/* used for both uniforms and storage buffer */
			void* _objects;
			size_t _object_size; /* The size of a single object element */
			size_t _object_count; /* Number of objects */

			const vulkan::ImageView *_image_view;
			Type _type;
	};
}
