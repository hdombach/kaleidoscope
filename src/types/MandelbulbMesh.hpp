#pragma once

#include <memory>

#include "Mesh.hpp"

namespace types {
	class MandelbulbMesh: public Mesh {
		public:
			using Ptr = std::unique_ptr<MandelbulbMesh>;

			static Ptr create(std::string const &name, uint32_t id);

			MandelbulbMesh(const MandelbulbMesh& other) = delete;
			MandelbulbMesh(MandelbulbMesh &&other) = default;
			MandelbulbMesh& operator=(const MandelbulbMesh& other) = delete;
			MandelbulbMesh& operator=(MandelbulbMesh&& other) = default;

			void destroy();
			~MandelbulbMesh();

			const_iterator begin() const override;
			const_iterator end() const override;
			uint32_t id() const override;
			size_t size() const override;
			void set_name(std::string const &name) override;
			std::string const &name() const override;

			bool is_de() const override { return true; }
			std::string const &de() const override;

		private:
			MandelbulbMesh() = default;

			uint32_t _id;
			std::string _name;
			std::string _de;
	};
}
