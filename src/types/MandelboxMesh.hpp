#pragma once

#include <memory>

#include "Mesh.hpp"

namespace types {
	class MandelboxMesh: public Mesh {
		public:
			using Ptr = std::unique_ptr<MandelboxMesh>;

			static Ptr create(std::string const &name, uint32_t id);

			MandelboxMesh(const MandelboxMesh& other) = delete;
			MandelboxMesh(MandelboxMesh &&other) = default;
			MandelboxMesh& operator=(const MandelboxMesh& other) = delete;
			MandelboxMesh& operator=(MandelboxMesh&& other) = default;

			void destroy();
			~MandelboxMesh();

			const_iterator begin() const override;
			const_iterator end() const override;
			uint32_t id() const override;
			size_t size() const override;
			void set_name(std::string const &name) override;
			std::string const &name() const override;

			bool is_de() const override { return true; }
			std::string const &de() const override;

		private:
			MandelboxMesh() = default;

			uint32_t _id;
			std::string _name;
			std::string _de;
	};
}
