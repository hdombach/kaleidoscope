#pragma once

#include <glm/glm.hpp>

#include "../util/result.hpp"
#include "../util/errors.hpp"
#include "../vulkan/DescriptorSet.hpp"

#include "MappedUniform.hpp"
#include "../types/Node.hpp"

namespace vulkan {
	class Scene;
	class PrevPass;

	class PrevPassNode {
		public:
			PrevPassNode() = default;

			static util::Result<PrevPassNode, KError> create(
					Scene &scene,
					PrevPass &preview_pass,
					const Node *node);

			PrevPassNode(const PrevPassNode& other) = delete;
			PrevPassNode(PrevPassNode &&other) = default;
			PrevPassNode& operator=(const PrevPassNode& other) = delete;
			PrevPassNode& operator=(PrevPassNode&& other) = default;

			void destroy();

			~PrevPassNode() { destroy(); }

			DescriptorSets &descriptor_set() { return _descriptor_set; }

			void update();

		private:
			uint32_t _id;
			const vulkan::Node *_node;
			DescriptorSets _descriptor_set;
			Uniform _uniform;
	};
}
