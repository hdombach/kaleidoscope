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
				Color3,
				Float,
			};

			static ShaderResource create_primitive(std::string name, float val);
			static ShaderResource create_primitive(std::string name, glm::mat4 mat);
			static ShaderResource create_primitive(std::string name, glm::vec3 vec);
			static ShaderResource create_color(std::string name, glm::vec3 color);

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
			util::Result<glm::mat4, void> as_mat4() const;
			util::Result<void, KError> set_vec3(glm::vec3 const &val);
			util::Result<glm::vec3 const &, void> as_vec3() const;
			util::Result<void, KError> set_color3(glm::vec3 const &vale);
			util::Result<glm::vec3 const &, void> as_color3() const;
			util::Result<void, KError> set_float(float val);
			util::Result<float, void> as_float() const;

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

			ShaderResources(ShaderResources const *parent = nullptr);

			void add_resource(ShaderResource resource);

			std::vector<ShaderResource const *> get() const;
			ShaderResource const *get(std::string name) const;

			ShaderResource const & operator[](std::string name) const { return *get(name); }

			void set_mat4(std::string const &name, glm::mat4 const &val);
			void set_vec3(std::string const &name, glm::vec3 const &val);
			void set_color3(std::string const &name, glm::vec3 const &val);
			void set_float(std::string const &name, float &val);

			/* Number of resources */
			size_t size() const { return _resources.size(); }

			/* Size (with padding) of vulkan object */
			size_t range() const { return _range; }

			util::Result<vulkan::Uniform, KError> create_prim_uniform() const;
			size_t update_prim_uniform(vulkan::Uniform &uniform) const;
			size_t update_prim_uniform(void *data) const;

			bool dirty_bits() const;
			void clear_dirty_bits();
		private:
			Container _resources;
			size_t _range;
			ShaderResources const *_parent;

		private:
			static size_t _calc_alignment(size_t alignment, size_t cur_offset);
			size_t _calc_range() const;
	};
}
