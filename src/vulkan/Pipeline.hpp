#pragma once

#include <vector>
#include <tuple>

#include <vulkan/vulkan_core.h>

#include "Image.hpp"
#include "util/result.hpp"
#include "Error.hpp"
#include "RenderPass.hpp"
#include "DescriptorSet.hpp"
#include "vulkan/Shader.hpp"

namespace vulkan {
	class Pipeline {
		public:
			enum class Type {
				GRAPHICS,
				COMPUTE,
			};

			using Attachments = std::vector<std::vector<DescAttachment>>;
		public:
			Pipeline() = default;

			static util::Result<Pipeline, Error> create_graphics(
				Shader const &vertex_shader,
				Shader const &fragment_shader,
				RenderPass const &render_pass,
				Attachments const &attachments
			);

			Pipeline(Pipeline const &pipeline) = delete;
			Pipeline(Pipeline &&pipeline);
			Pipeline &operator=(Pipeline const &pipeline) = delete;
			Pipeline &operator=(Pipeline &&pipeline);

			void destroy();
			~Pipeline();

			VkPipeline pipeline() const;
			VkPipelineLayout pipeline_layout() const;

			std::vector<DescriptorSetLayout> const &layouts();
			std::vector<VkDescriptorSetLayout> vk_layouts();

			Attachments const &attachments() const;

			RenderPass const &render_pass() const;

		private:
			Type _type;
			VkExtent2D _size;

			RenderPass const *_render_pass = nullptr;

			VkPipeline _pipeline = nullptr;
			VkPipelineLayout _pipeline_layout = nullptr;

			Attachments _attachments;

			std::vector<DescriptorSetLayout> _descriptor_set_layouts;
	};
}
