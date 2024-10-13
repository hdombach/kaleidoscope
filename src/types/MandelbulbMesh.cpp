#include "MandelbulbMesh.hpp"

namespace types {
	MandelbulbMesh::Ptr MandelbulbMesh::create() {
		auto result = MandelbulbMesh::Ptr(new MandelbulbMesh());
		result->_de =
			"float de(vec3 pos, inout float depth) {\n"
			"	vec3 z = pos;\n"
			"	float dr = 1;\n"
			"	float r = 0;\n"
			"	float power = 8.0;\n"
			"	float bailout = 2.0f;\n"
			"	uint iterations = 20;\n"
			"	float orbigLife;\n"
			"	depth = 0;\n"
			"	for (uint i = 0; i < iterations; i++) {\n"
			"		r = length(z);\n"
			"		if (r > bailout) {\n"
			"			orbitLife = i;\n"
			"			break;\n"
			"		}\n"
			"		//convert to polar\n"
			"		float theta = acos(z.z / r);\n"
			"		float phi = atan(z.y / z.x);\n"
			"		dr = pow(r, power - 1) * power * dr + 1;\n"
			"		//scale and rotate the point\n"
			"		float zr = pow(r, power);\n"
			"		theta = theta * power;\n"
			"		phi = phi * power;\n"
			"		//convert back to cartesian\n"
			"		z = zr * vec3(sin(theta) * cos(phi), sin(phi) * sin(theta), cos(theta));\n"
			"		z += pos;\n"
			"		orbitLife = i;\n"
			"	}\n"
			"	orbitLife = orbitLife / iterations;\n"
			"	return 0.5 * log(r) * r / dr;\n"
			"}";
		return std::move(result);
	}

	void MandelbulbMesh::destroy() { }

	MandelbulbMesh::~MandelbulbMesh() {
		destroy();
	}

	MandelbulbMesh::const_iterator MandelbulbMesh::begin() const { return nullptr; }
	MandelbulbMesh::const_iterator MandelbulbMesh::end() const { return nullptr; }
	uint32_t MandelbulbMesh::id() const { return _id; }
	size_t MandelbulbMesh::size() const { return 0; }
	void MandelbulbMesh::set_name(std::string const &name) { _name = name; }
	std::string const &MandelbulbMesh::name() const { return _name; }

	std::string const &MandelbulbMesh::de() const {
		return _de;
	}
}
