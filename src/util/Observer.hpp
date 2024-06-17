#pragma once

#include <cstdint>
namespace util {
	class Observer {
		public:
			virtual void obs_create(uint32_t id) = 0;
			virtual void obs_update(uint32_t id) = 0;
			virtual void obs_remove(uint32_t id) = 0;
	};
}
