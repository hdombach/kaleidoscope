#pragma once

#include <vulkan/vulkan_core.h>

#include "util/result.hpp"
#include "Error.hpp"

namespace vulkan {
	class Sampler {
		public:
			static util::Result<Sampler, Error> create_linear();
			static util::Result<Sampler, Error> create_nearest();

			Sampler();

			Sampler(const Sampler& other) = delete;
			Sampler(Sampler &&other);
			Sampler& operator=(const Sampler& other) = delete;
			Sampler& operator=(Sampler &&other);

			void destroy();
			~Sampler() { destroy(); }

			bool exists() const;

			operator bool() const { return exists(); }

			VkSampler &get();
			VkSampler const &get() const;

			VkSampler &operator*() { return get(); }
			VkSampler const& operator*() const { return get(); }

		private:
			VkSampler _sampler;

			static VkSamplerCreateInfo _default_template();
	};
}
