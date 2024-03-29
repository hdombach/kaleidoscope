#pragma once

#include "material.hpp"
#include "materialFactory.hpp"
#include "texture.hpp"
#include "vulkan/vulkan_core.h"
#include <vector>

namespace vulkan {
	class TextureMaterial: public Material {
		public:
			~TextureMaterial();
			VkPipelineLayout pipelineLayout() const;
			VkPipeline pipeline() const;
			std::vector<VkDescriptorSet> getDescriptorSet(uint32_t frameIndex) const;
		protected:
			friend MaterialFactory;

			TextureMaterial(MaterialFactory &materialFactory, Texture *texture);

			void createDescriptorSetLayout_();
			void createDescriptorSets_(MaterialFactory &materialFactory);
			void createPipeline_(MaterialFactory &materialFactory);

			Texture *texture_; // does not own
			MaterialFactory const &materialFactory_;

			VkPipelineLayout pipelineLayout_;
			VkPipeline pipeline_;
			//TODO: is possible to shader descriptor set layouts and even 
			//descriptor sets between materials.
			std::vector<VkDescriptorSet> descriptorSets_;
			VkDescriptorSetLayout descriptorSetLayout_;
	};
}
