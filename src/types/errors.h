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

}
