#pragma once
#include "system.h"

struct GLFWwindow;

namespace impuls
{
	struct queue_family_indices
	{
		std::optional<uint32_t> m_graphics_family;
		std::optional<uint32_t> m_present_family;

		bool is_complete() const
		{
			return m_graphics_family.has_value() && m_present_family.has_value();
		}
	};

	struct swapchain_support_details
	{
		VkSurfaceCapabilitiesKHR m_capabilities;
		std::vector<VkSurfaceFormatKHR> m_formats;
		std::vector<VkPresentModeKHR> m_present_modes;
	};

	struct vulkan_renderer_system : public i_system<>
	{
		virtual void on_begin_simulation() override;
		virtual void on_end_simulation() override;
		virtual void on_tick(float in_time_delta) override;

		void draw_frame();

		bool create_window();
		bool create_instance();
		bool create_debug_messenger();
		bool create_surface();
		bool create_physical_device();
		bool create_logical_device();
		bool create_swapchain();
		bool create_image_views();
		bool create_render_pass();
		bool create_graphics_pipeline();
		bool create_framebuffers();
		bool create_command_pool();
		bool create_vertex_buffer();
		bool create_command_buffers();
		bool create_syncers();

		bool recreate_swapchain();

		void cleanup_swapchain();
		void cleanup_vulkan();

		bool check_validation_layer_support() const;
		bool check_physical_device_extension_support(VkPhysicalDevice in_device) const;
		
		std::vector<const char*> get_required_extensions() const;
		ui32 physical_device_score(VkPhysicalDevice in_device) const;
		queue_family_indices find_queue_families(VkPhysicalDevice in_device) const;

		swapchain_support_details query_swapchain_support(VkPhysicalDevice in_device) const;
		VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& in_available_formats) const;
		VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR>& in_available_present_modes) const;
		VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& in_capabilities) const;

		VkShaderModule create_shader_module(const std::vector<char>& in_code);

		GLFWwindow* m_window = nullptr;
		VkInstance m_instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;
		
		VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		
		VkQueue m_graphics_queue = VK_NULL_HANDLE;
		VkQueue m_present_queue = VK_NULL_HANDLE;

		VkSurfaceKHR m_surface = VK_NULL_HANDLE;

		VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
		std::vector<VkImage> m_swapchain_images;
		std::vector<VkImageView> m_swapchain_image_views;
		std::vector<VkFramebuffer> m_swapchain_framebuffers;
		VkFormat m_swapchain_image_format;
		VkExtent2D m_swapchain_extent;

		VkRenderPass m_render_pass = VK_NULL_HANDLE;
		VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
		VkPipeline m_graphics_pipeline = VK_NULL_HANDLE;

		VkCommandPool m_command_pool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> m_command_buffers;

		std::vector<VkSemaphore> m_image_available_semaphores;
		std::vector<VkSemaphore> m_render_finished_semaphores;
		std::vector<VkFence> m_in_flight_fences;

		VkBuffer m_vertex_buffer = VK_NULL_HANDLE;

		ui8 m_current_frame = 0;
		bool m_framebuffer_resized = false;
		bool m_successfully_initialized = false;
	};
}

