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
				Float,
			};

			static ShaderResource create_primitive(std::string name, float &val);
			static ShaderResource create_primitive(std::string name, glm::mat4 &mat);
			static ShaderResource create_primitive(std::string name, glm::vec3 &vec);

			static ShaderResource create_image(std::string name, vulkan::ImageView const &image_view);

			~ShaderResource() = default;

			void const *primitive() const { return _primitive; }
			size_t primitive_size() const { return _primitive_size; }

			Type type() const { return _type; }

			bool is_primitive() const;

			vulkan::ImageView const &image_view() const { return *_image_view; }

			std::string const &name() const { return _name; }
			std::string const &declaration() const { return _declaration; }
			size_t alignment() const { return _alignment; }

			util::Result<glm::mat4&, void> as_mat4();
			util::Result<glm::vec3&, void> as_vec3();
			util::Result<float&, void> as_float();

		private:
			ShaderResource(std::string &name, Type type);

			std::string _name;
			std::string _declaration;

			void* _primitive;
			size_t _primitive_size;
			size_t _alignment;

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

			void add_resource(ShaderResource resource);

			ShaderResource &get(size_t index) { return _resources[index]; }
			ShaderResource const &get(size_t index) const { return _resources[index]; }

			ShaderResource& operator[](size_t index) { return get(index); }
			ShaderResource const& operator[](size_t index) const { return get(index); }

			util::Result<ShaderResource&, void> get(std::string name);
			util::Result<ShaderResource const&, void> get(std::string name) const;

			ShaderResource& operator[](std::string name) { return get(name).value(); }
			ShaderResource const & operator[](std::string name) const { return get(name).value(); }

			bool contains(std::string name) const;

			iterator begin() { return _resources.begin(); }
			iterator end() { return _resources.end(); }
			const_iterator begin() const { return _resources.begin(); }
			const_iterator end() const { return _resources.end(); }

			/* Number of resources */
			size_t size() const { return _resources.size(); }

			/* Size (with padding) of vulkan object */
			size_t range() const { return _range; }

			util::Result<vulkan::Uniform, KError> create_prim_uniform() const;
			size_t update_prim_uniform(vulkan::Uniform &uniform) const;
			size_t update_prim_uniform(
					vulkan::Uniform &uniform,
					const_iterator begin,
					const_iterator end) const;
			size_t update_prim_uniform(
					void *data,
					const_iterator begin,
					const_iterator end) const;
		private:
			Container _resources;
			size_t _range;

			static size_t _calc_alignment(size_t alignment, size_t cur_offset);

			size_t _calc_range() const;
	};
}
