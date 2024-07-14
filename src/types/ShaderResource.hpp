#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "../vulkan/ImageView.hpp"
#include "../vulkan/MappedUniform.hpp"

namespace types {
	class ShaderResource {
		public:
			enum class Type {
				Unknown,
				Image,
				Mat4,
				Vec3,
			};

			static ShaderResource create_primitive(std::string name, glm::mat4 &mat);
			static ShaderResource create_primitive(std::string name, glm::vec3 &vec);

			static ShaderResource create_image(std::string name, vulkan::ImageView const &image_view);

			void const *primitive() const { return _primitive; }
			size_t primitive_size() const { return _primitive_size; }

			Type type() const { return _type; }

			bool is_primitive() const;

			vulkan::ImageView const &image_view() const { return *_image_view; }

			std::string const &name() const { return _name; }

			~ShaderResource() = default;

		private:
			ShaderResource(std::string &name, Type type);

			std::string _name;

			void* _primitive;
			size_t _primitive_size;

			const vulkan::ImageView *_image_view;
			Type _type;
	};

	class ShaderResources {
		private:
			using Container = std::vector<ShaderResource>;
		public:
			using iterator = Container::iterator;
			using const_iterator = Container::const_iterator;

			ShaderResources() = default;

			void add_resource(ShaderResource resource) { _resources.push_back(resource); }

			ShaderResource &get(size_t index) { return _resources[index]; }
			ShaderResource const &get(size_t index) const { return _resources[index]; }

			ShaderResource& operator[](size_t index) { return get(index); }
			ShaderResource const& operator[](size_t index) const { return get(index); }

			iterator begin() { return _resources.begin(); }
			iterator end() { return _resources.end(); }
			const_iterator begin() const { return _resources.begin(); }
			const_iterator end() const { return _resources.end(); }

			size_t size() const { return _resources.size(); }

			util::Result<vulkan::Uniform, KError> create_prim_uniform() const;
			void update_prim_uniform(vulkan::Uniform &uniform) const;
			void update_prim_uniform(
					vulkan::Uniform &uniform,
					const_iterator begin,
					const_iterator end) const;
		private:
			Container _resources;
	};
}
