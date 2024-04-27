#pragma once

#include <functional>
#include <vector>
#include <optional>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

#include "Semaphore.hpp"
#include "Fence.hpp"
#include "ImageView.hpp"
#include "Image.hpp"
#include "../util/errors.hpp"

namespace vulkan {
	class Semaphore;

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;

		bool isComplete() {
			return graphics_family.has_value() && present_family.has_value();
		}
	};

	class Graphics {
		public:
			struct SwapchainSupportDetails;

			static Graphics *DEFAULT;
			static void init_default(const char *name);
			static void delete_default();

			~Graphics();

			void wait_idle() const;
			GLFWwindow * window();

			VkSurfaceKHR const &surface() const;
			VkPhysicalDevice const &physical_device() const;
			VkDevice const &device() const;
			VkInstance const &instance() const;
			VkSampler main_texture_sampler() const;
			GLFWwindow* window() const;
			VkCommandPool command_pool() const;
			VkQueue graphics_queue() const;
			VkQueue present_queue() const;
			ImageView const &compute_image_view() const;
			SwapchainSupportDetails const &swapchain_support_details() const;
			util::Result<uint32_t, KError> find_memory_type(
					uint32_t type_filter,
					VkMemoryPropertyFlags properties);

			VkFormat find_supported_format(
					const std::vector<VkFormat>& candidates,
					VkImageTiling tiling,
					VkFormatFeatureFlags features) const;
			VkShaderModule create_shader_module(std::string const &code) const;
			QueueFamilyIndices find_queue_families() const;
			void create_buffer(
					VkDeviceSize size,
					VkBufferUsageFlags usage,
					VkMemoryPropertyFlags properties,
					VkBuffer& buffer,
					VkDeviceMemory& buffer_memory) const;
			void transition_image_layout(
					VkImage image,
					VkFormat format,
					VkImageLayout old_layout,
					VkImageLayout new_layout,
					uint32_t mipLevels) const;
			void copy_buffer_to_image(
					VkBuffer buffer,
					VkImage image,
					uint32_t width,
					uint32_t height) const;
			void generate_mipmaps(
					VkImage image,
					VkFormat image_format,
					int32_t tex_width,
					int32_t tex_height,
					uint32_t mip_levels) const;
			void copy_buffer(
					VkBuffer src_buffer,
					VkBuffer dst_buffer,
					VkDeviceSize size) const;
			void execute_single_time_command(std::function<void(VkCommandBuffer)>) const;

			struct SwapchainSupportDetails {
				VkSurfaceCapabilitiesKHR capabilities;
				std::vector<VkSurfaceFormatKHR> formats;
				std::vector<VkPresentModeKHR> present_modes;
			};

		private:
			Graphics() = default;
			void _init_window();
			void _init_vulkan();
			util::Result<void, KError> _create_instance();
			util::Result<void, KError> _setup_debug_messenger();
			void _pick_physical_device();
			bool _is_device_suitable(VkPhysicalDevice device);
			bool _check_device_extension_support(VkPhysicalDevice device);
			util::Result<void, KError> _create_logical_device();
			void _create_compute_descriptor_set_layout();
			void _create_compute_pipeline();
			void _create_command_pool();
			void _create_descriptor_pool();
			void _create_compute_descriptor_sets();
			void _create_compute_command_buffers();
			util::Result<void, KError> _create_sync_objects();
			void _create_surface();
			void _record_compute_command_buffer(VkCommandBuffer commandBuffer);
			void _create_texture_sampler();
			util::Result<void, KError> _create_compute_result_texture();

			bool _check_validation_layer_support();
			void _cleanup();

			VkResult _create_debug_utils_messenger_EXT(
					VkInstance instance,
					const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
					const VkAllocationCallbacks* p_allocater,
					VkDebugUtilsMessengerEXT* p_debug_messenger);
			void _destroy_debug_utils_messenger_EXT(
					VkInstance instance,
					VkDebugUtilsMessengerEXT debug_messenger,
					const VkAllocationCallbacks* p_allocator);
			static void _framebuffer_resize_callback(
					GLFWwindow* window,
					 int width,
					 int height);
			std::vector<const char*> _get_required_extensions();
			void _populate_debug_messenger_create_info(
					VkDebugUtilsMessengerCreateInfoEXT& create_info);
			QueueFamilyIndices _find_queue_families(VkPhysicalDevice device) const;
			SwapchainSupportDetails _query_swap_chain_support(VkPhysicalDevice device);
			VkSurfaceFormatKHR _choose_swap_surface_format(
					const std::vector<VkSurfaceFormatKHR>& available_formats);
			VkPresentModeKHR _choose_swap_present_mode(
					const std::vector<VkPresentModeKHR>& available_present_modes);
			VkShaderModule _create_shader_module(const std::string& code) const;
			uint32_t _find_memory_type(
					uint32_t type_filter,
					VkMemoryPropertyFlags properties) const;
			void _create_buffer(
					VkDeviceSize size,
					VkBufferUsageFlags usage,
					VkMemoryPropertyFlags properties,
					VkBuffer& buffer,
					VkDeviceMemory& buffer_memory) const;
			void _copy_buffer(
					VkBuffer src_buffer,
					VkBuffer dst_buffer,
					VkDeviceSize size) const;
			VkCommandBuffer _begin_single_time_commands() const;
			void _end_single_time_commands(VkCommandBuffer command_buffer) const;
			void _transition_image_layout(
					VkImage image,
					VkFormat format,
					VkImageLayout old_layout,
					VkImageLayout new_layout,
					uint32_t mip_levels) const;
			void _copy_buffer_to_image(
					VkBuffer buffer,
					VkImage image,
					uint32_t width,
					uint32_t height) const;
			VkFormat _find_supported_format(
					const std::vector<VkFormat>& candidates,
					VkImageTiling tiling,
					VkFormatFeatureFlags features) const;
			VkFormat _find_depth_format();
			bool _has_stencil_component(VkFormat format) const;
			void _generate_mipmaps(
					VkImage image,
					VkFormat image_format,
					int32_t tex_width,
					int32_t tex_height,
					uint32_t mip_levels) const;

			static VKAPI_ATTR VkBool32 VKAPI_CALL _debug_callback(
				VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
				VkDebugUtilsMessageTypeFlagsEXT message_typedata,
				const VkDebugUtilsMessengerCallbackDataEXT * p_callback_data,
				void* p_user_data);


			const char *_name;
			GLFWwindow* _window;
			VkInstance _instance;
			VkDebugUtilsMessengerEXT _debug_messenger;
			VkPhysicalDevice _physical_device;
			VkDevice _device;
			VkQueue _graphics_queue;
			VkQueue _present_queue;
			VkQueue _compute_queue;
			VkSurfaceKHR _surface;
			VkDescriptorSetLayout _compute_descriptor_set_layout;
			VkDescriptorPool _descriptor_pool;
			std::vector<VkDescriptorSet> _compute_descriptor_sets;
			VkPipelineLayout _compute_pipeline_layout;
			VkPipeline _compute_pipeline;
			VkCommandPool _command_pool;
			uint32_t _mip_levels;
			VkSampler _texture_sampler;
			Image _compute_result_image;
			ImageView _compute_result_image_view;
			std::vector<VkCommandBuffer> _compute_command_buffers;
			std::vector<Semaphore> _compute_finished_semaphores;
			std::vector<Fence> _compute_in_flight_fences;
			SwapchainSupportDetails _swapchain_support_details;

			//imgui stuff
			bool _framebuffer_resized = false;
			uint32_t _current_frame = 0;
	};
}
