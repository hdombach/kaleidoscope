#pragma once

#include <glm/glm.hpp>

#include "vulkan/DescriptorSet.hpp"
#include "vulkan/MappedUniform.hpp"
#include "util/result.hpp"
#include "util/errors.hpp"
#include "types/Node.hpp"

namespace vulkan {
	class Scene;
	class PrevPass;

	class PrevPassNode {
		public:
			struct VImpl {
				alignas(4) uint32_t mesh_id;
				alignas(16) glm::vec3 position;

				static VImpl create_empty();

				static constexpr const char *declaration() {
					return
						"struct Node {\n"
						"\tuint mesh_id;\n"
						"\tvec3 position;\n"
						"};\n";
				};
			} __attribute__((packed));

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

			operator bool() { return _node; }

			DescriptorSets &descriptor_set() { return _descriptor_set; }

			void update();

			VImpl vimpl();

			bool is_de();

		private:
			uint32_t _id;
			const vulkan::Node *_node;
			DescriptorSets _descriptor_set;
			Uniform _uniform;
			PrevPass *_prev_pass;

		private:
			util::Result<void, KError> _create_descriptor_sets();
	};
}
