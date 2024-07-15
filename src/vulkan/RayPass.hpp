#include <memory>

#include "DescriptorPool.hpp"
#include "DescriptorSet.hpp"
#include "Fence.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include "Semaphore.hpp"
#include "Texture.hpp"
#include "UniformBufferObject.hpp"
#include "../types/Node.hpp"

namespace vulkan {
	class RayPass: public Texture {
		public:
			using Ptr = std::unique_ptr<RayPass>;
			static util::Result<Ptr, KError> create(VkExtent2D size);

			~RayPass();

			RayPass(const RayPass& other) = delete;
			RayPass(RayPass &&other);
			RayPass& operator=(const RayPass& other) = delete;
			RayPass& operator=(RayPass&& other);
			RayPass();

			VkDescriptorSet get_descriptor_set() override;
			ImageView const &image_view() override;
			void submit(Node &node);
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

			util::Result<void, KError> _create_descriptor_sets(Node &node);
	};
}
