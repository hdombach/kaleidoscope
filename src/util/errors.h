#pragma once

#include <ostream>
#include <string>
namespace errors {
	struct TextureAlreadyExists {
		std::string textureName;
	};

	struct InvalidImageFile {
		std::string filename;

	};
	inline std::ostream &operator <<(std::ostream &os, InvalidImageFile error) {
		os << "File " << error.filename << " is not an image";
		return os;
	}

	struct InvalidMeshFile {
		std::string filename;
	};
	inline std::ostream &operator <<(std::ostream &os, InvalidMeshFile error) {
		os << "File " << error.filename << " is not a mesh";
		return os;
	}

	struct MeshAlreadyExists {
		std::string meshName;
	};

}
