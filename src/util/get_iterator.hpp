#pragma once

#include <ranges>

namespace util {
	template<std::ranges::range R>
		using get_iterator = decltype(std::declval<R&>().begin());
}
