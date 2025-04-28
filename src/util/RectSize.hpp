#pragma once

#include <cstdint>

namespace util {
	struct RectSize {
		RectSize() = default;
		RectSize(uint32_t width, uint32_t height): w(width), h(height) {}

		uint32_t w = 0;
		uint32_t h = 0;
	};
}
