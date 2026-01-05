#pragma once

#include <cstdint>
#include <limits>

#include <vulkan/vulkan_core.h>

#include "util/result.hpp"
#include "DescriptorPool.hpp"
#include "MappedUniform.hpp"
#include "StaticBuffer.hpp"
#include "Error.hpp"
#include "Image.hpp"
#include "DescAttachment.hpp"

/**
 * @file Tools for building and storing descriptor sets
 */

namespace vulkan {
	static const uint32_t DESCRIPTOR_BINDING_UNUSED = std::numeric_limits<uint32_t>::max();
	/**
	 * @brief String representation of the descriptor type
	 */
	const char *descriptor_type_str(VkDescriptorType const &t);

	/**
	 * @brief Describes what types are used in a pipeline along with their order
	 */
	class DescriptorSetLayout {
		public:
			DescriptorSetLayout() = default;

			/**
			 * @brief Creates a new descriptor set layout based off the list of attachments
			 *
			 * Techincally, the internal state of attachments is modified. This is becuase
			 * attachments stores internal structs used for construction structs.
			 */
			static util::Result<DescriptorSetLayout, Error> create(
				std::vector<DescAttachment> const &attachments
			);

			DescriptorSetLayout(DescriptorSetLayout const &other) = delete;
			DescriptorSetLayout(DescriptorSetLayout &&other);
			DescriptorSetLayout& operator=(DescriptorSetLayout const &other) = delete;
			DescriptorSetLayout& operator=(DescriptorSetLayout &&other);

			void destroy();
			~DescriptorSetLayout();

			/**
			 * @brief Is the DescriptorSetLayout initialized
			 */
			bool has_value() const;
			/**
			 * @brief Is the DescriptorSetLayout initialized
			 */
			operator bool() const;

			/**
			 * @brief The core vulkan layout
			 */
			VkDescriptorSetLayout const &layout() const;

			/**
			 * @brief The underlysing set of attachments used
			 */
			std::vector<DescAttachment> const &desc_attachments() const;

		private:
			VkDescriptorSetLayout _layout = nullptr;
			std::vector<DescAttachment> _desc_attachments;
	};


	/**
	 * @brief A collection of resources that can be used by a pipeline
	 */
	class DescriptorSets {
		public:
			/**
			 * @brief Creates an uninitialized DescriptorSet
			 */
			DescriptorSets() = default;

			/**
			 * @brief Creates a DescriptorSet with information information provided by the attachments
			 */
			static util::Result<DescriptorSets, Error> create(
				std::vector<DescAttachment> &attachments,
				DescriptorSetLayout const &layout,
				DescriptorPool const &pool
			);

			DescriptorSets(const DescriptorSets &other) = delete;
			DescriptorSets(DescriptorSets &&other);
			DescriptorSets &operator=(DescriptorSets const &other) = delete;
			DescriptorSets &operator=(DescriptorSets &&other);

			void destroy();
			~DescriptorSets();

			/**
			 * @brief Is the DescriptorSet initialized
			 */
			bool has_value() const;
			/**
			 * @brief Is the DescriptorSet initialized
			 */
			operator bool() const;

			/**
			 * @brief Gets the descriptor set for a specific frame
			 */
			const VkDescriptorSet descriptor_set(uint32_t frame_index = 0) const;

		private:
			std::vector<VkDescriptorSet> _descriptor_sets;
			DescriptorPool const *_descriptor_pool;
	};
}
