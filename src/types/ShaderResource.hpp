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

			static ShaderResource create_primitive(std::string name, float val);
			static ShaderResource create_primitive(std::string name, glm::mat4 mat);
			static ShaderResource create_primitive(std::string name, glm::vec3 vec);

			static ShaderResource create_image(std::string name, vulkan::ImageView const &image_view);

			~ShaderResource() = default;

			Type type() const { return _type; }

			bool is_primitive() const;
			size_t primitive_size() const { return _primitive_size; }
			void const *data() const { return &_as_float; }

			std::string const &name() const { return _name; }
			std::string const &declaration() const { return _declaration; }
			size_t alignment() const { return _alignment; }

			util::Result<vulkan::ImageView const &, void> as_image() const;
			util::Result<void, KError> set_mat4(glm::mat4 const &val);
			util::Result<glm::mat4 const&, void> as_mat4() const;
			util::Result<void, KError> set_vec3(glm::vec3 const &val);
			util::Result<glm::vec3 const &, void> as_vec3() const;
			util::Result<void, KError> set_float(float val);
			util::Result<float const &, void> as_float() const;

			bool const dirty_bit() const { return _dirty_bit; }
			void clear_dirty_bit() { _dirty_bit = false; }

		private:
			ShaderResource(std::string &name, Type type);
			void _set_dirty_bit();

		private:
			std::string _name;
			std::string _declaration;

			union {
				float _as_float;
				glm::mat4 _as_mat4;
				glm::vec3 _as_vec3;
				const vulkan::ImageView *_as_image;
			};

			size_t _alignment;
			size_t _primitive_size;

			Type _type;
			bool _dirty_bit;
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

			bool dirty_bits() const;
			void clear_dirty_bits();
		private:
			Container _resources;
			size_t _range;

			static size_t _calc_alignment(size_t alignment, size_t cur_offset);

			size_t _calc_range() const;
	};
}
