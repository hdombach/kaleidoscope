#include "DescriptorPool.hpp"
#include "DescriptorSet.hpp"
#include "Fence.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include "Semaphore.hpp"
#include "Texture.hpp"
#include "UniformBufferObject.hpp"
#include <memory>

namespace vulkan {
	class RaytraceRenderPass: public Texture {
		public:
			using Ptr = std::unique_ptr<RaytraceRenderPass>;
			static util::Result<Ptr, KError> create(VkExtent2D size);

			~RaytraceRenderPass();

			RaytraceRenderPass(const RaytraceRenderPass& other) = delete;
			RaytraceRenderPass(RaytraceRenderPass &&other);
			RaytraceRenderPass& operator=(const RaytraceRenderPass& other) = delete;
			RaytraceRenderPass& operator=(RaytraceRenderPass&& other);
			RaytraceRenderPass();

			VkDescriptorSet get_descriptor_set() override;
			ImageView const &image_view() override;
			void submit();
			MappedComputeUniform &current_uniform_buffer();

		private:
			VkExtent2D _size;
			Image _result_image;
			ImageView _result_image_view;
			Fence _pass_fence;
			Semaphore _pass_semaphore;
			DescriptorPool _descriptor_pool;
			DescriptorSets _descriptor_set;
			VkDescriptorSet _imgui_descriptor_set;
			VkPipelineLayout _pipeline_layout;
			VkPipeline _pipeline;
			VkCommandBuffer _command_buffer;
			MappedComputeUniform _mapped_uniform;
	};
}
