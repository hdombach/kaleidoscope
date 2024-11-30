#include "MandelboxMesh.hpp"

namespace types {
	MandelboxMesh::Ptr MandelboxMesh::create(std::string const &name, uint32_t id) {
		auto result = Ptr(new MandelboxMesh());
		result->set_name(name);
		result->_id = id;
		result->_de =
			"pos *= 8;\n"
			"vec3 offset = pos;\n"
			"float dr = 1.0f;\n"
			"float scale = 2.0f;\n"
			"int iterations = 40;\n"
			"for (int n = 0; n < iterations; n++) {\n"
			"	//boxFold\n"
			"	float limit = 1;\n"
			"	pos = clamp(pos, -limit, limit) * 2 - pos;\n"
			"	//sphereFold\n"
			"	float fixedRadius2 = 2.0;\n"
			"	float minRadius2 = 0.5;\n"
			"	\n"
			"	float r2 = dot(pos, pos);\n"
			"	if (r2 < minRadius2) {\n"
			"		float temp = (fixedRadius2 / minRadius2);\n"
			"		pos = pos * temp;\n"
			"		dr = dr * temp;\n"
			"	} else if (r2 < fixedRadius2) {\n"
			"		float temp = fixedRadius2 / r2;\n"
			"		pos = pos * temp;\n"
			"		dr = dr * temp;\n"
			"	}\n"
			"	pos = scale * pos + offset;\n"
			"	dr = dr * abs(scale) + 1.0;\n"
			"}\n"
			"float r = length(pos);\n"
			"return r / abs(dr) / 8;\n";
		return std::move(result);
	}

	void MandelboxMesh::destroy() {
		_id = 0;
		_name.clear();
		_de.clear();
	}

	MandelboxMesh::~MandelboxMesh() {
		destroy();
	}

	MandelboxMesh::const_iterator MandelboxMesh::begin() const { return nullptr; }
	MandelboxMesh::const_iterator MandelboxMesh::end() const { return nullptr; }
	uint32_t MandelboxMesh::id() const { return _id; }
	size_t MandelboxMesh::size() const { return 0; }
	void MandelboxMesh::set_name(std::string const &name) { _name = name; }
	std::string const &MandelboxMesh::name() const { return _name; }

	std::string const &MandelboxMesh::de() const { return _de; }
}
