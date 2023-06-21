#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan {
	class Material {
		public:
			virtual ~Material() = default;
			virtual VkPipelineLayout pipelineLayout() const = 0;
			virtual VkPipeline pipeline() const = 0;
			virtual std::vector<VkDescriptorSet> getDescriptorSet(uint32_t frameIndex) const = 0;
	};
}
