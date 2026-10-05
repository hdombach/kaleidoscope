#pragma once

#include <ranges>
#include <utility>

namespace util {
	template<std::ranges::range R>
		using get_iterator = decltype(std::declval<R&>().begin());
}
