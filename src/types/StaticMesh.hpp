#pragma once

#include <vector>
#include <memory>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "../util/errors.hpp"
#include "../util/result.hpp"
#include "../types/Mesh.hpp"
#include "../vulkan/Vertex.hpp"


namespace types {
	class StaticMesh: public Mesh {
		public:
			using Ptr = std::unique_ptr<StaticMesh>;

			static util::Result<Ptr, KError> from_file(uint32_t id, std::string const &url);
			static Ptr create_square(std::string const &name, uint32_t id);
			static Ptr from_vertices(std::string const &name, uint32_t id, std::vector<vulkan::Vertex> const &vertices);

			StaticMesh(const StaticMesh& other) = delete;
			StaticMesh(StaticMesh &&other) = default;
			StaticMesh& operator=(const StaticMesh& other) = delete;
			StaticMesh& operator=(StaticMesh&& other) = default;

			void destroy();
			~StaticMesh();

			const_iterator begin() const override;
			const_iterator end() const override;
			uint32_t id() const override;
			size_t size() const override;
			void set_name(std::string const &name) override;
			std::string const &name() const override;

		private:
			StaticMesh() = default;

			std::vector<vulkan::Vertex> _vertices;
			uint32_t _id;
			std::string _name;
	};
}
