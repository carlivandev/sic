#include "stdafx.h"
#include "vulkan_renderer_system.h"

const impuls::ui16 width = 1024;
const impuls::ui16 height = 512;

const impuls::i8 MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validation_layers =
{
	"VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> device_extensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef _DEBUG
const bool enable_validation_layers = true;
#else
const bool enable_validation_layers = false;
#endif

namespace impuls
{
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr)
		{
			func(instance, debugMessenger, pAllocator);
		}
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback
	(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData
	)
	{
		messageSeverity;
		messageType;
		pUserData;

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

	static std::vector<char> read_file(const std::string& in_filename)
	{
		std::ifstream file(in_filename, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			return std::vector<char>();

		const ui16 file_size = (ui16)file.tellg();
		std::vector<char> buffer(file_size);

		file.seekg(0);
		file.read(buffer.data(), file_size);
		file.close();

		return buffer;
	}

	static void framebuffer_resize_callback(GLFWwindow* in_window, int in_width, int in_height)
	{
		in_width; in_height;
		auto app = reinterpret_cast<vulkan_renderer_system*>(glfwGetWindowUserPointer(in_window));
		app->m_framebuffer_resized = true;
	}

	struct vertex
	{
		glm::vec2 m_pos;
		glm::vec3 m_color;

		static VkVertexInputBindingDescription get_binding_description()
		{
			VkVertexInputBindingDescription binding_description = {};
			binding_description.binding = 0;
			binding_description.stride = sizeof(vertex);
			binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return binding_description;
		}

		static std::array<VkVertexInputAttributeDescription, 2> get_attribute_descriptions()
		{
			std::array<VkVertexInputAttributeDescription, 2> attribute_descriptions = {};

			attribute_descriptions[0].binding = 0;
			attribute_descriptions[0].location = 0;
			attribute_descriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attribute_descriptions[0].offset = offsetof(vertex, m_pos);

			attribute_descriptions[1].binding = 0;
			attribute_descriptions[1].location = 1;
			attribute_descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attribute_descriptions[1].offset = offsetof(vertex, m_color);

			return attribute_descriptions;
		}
	};

	const std::vector<vertex> vertices =
	{
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};
}

void impuls::vulkan_renderer_system::on_begin_simulation()
{
	m_successfully_initialized = true;

	if (!create_window() ||
		!create_instance() ||
		!create_debug_messenger() ||
		!create_surface() ||
		!create_physical_device() ||
		!create_logical_device() ||
		!create_swapchain() ||
		!create_image_views() ||
		!create_render_pass() ||
		!create_graphics_pipeline() ||
		!create_framebuffers() ||
		!create_command_pool() ||
		!create_vertex_buffer() ||
		!create_command_buffers() ||
		!create_syncers()
		)
	{
		m_successfully_initialized = false;
		cleanup_vulkan();
	}
}

void impuls::vulkan_renderer_system::on_end_simulation()
{
	vkDeviceWaitIdle(m_device);

	cleanup_vulkan();
}

void impuls::vulkan_renderer_system::on_tick(float)
{
	if (!m_successfully_initialized)
		return;

	if (glfwWindowShouldClose(m_window))
	{
		m_world->destroy();
	}
	else
	{
		glfwPollEvents();
		draw_frame();
	}
}

void impuls::vulkan_renderer_system::draw_frame()
{
	vkWaitForFences(m_device, 1, &m_in_flight_fences[m_current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	ui32 image_index;
	VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, std::numeric_limits<ui64>::max(), m_image_available_semaphores[m_current_frame], VK_NULL_HANDLE, &image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreate_swapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		std::cout << "failed to acquire swap chain image!\n";
		
		vkResetFences(m_device, 1, &m_in_flight_fences[m_current_frame]);

		cleanup_vulkan();
		m_successfully_initialized = false;
		return;
	}

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { m_image_available_semaphores[m_current_frame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_command_buffers[image_index];

	VkSemaphore signal_semaphores[] = { m_render_finished_semaphores[m_current_frame] };
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = signal_semaphores;

	vkResetFences(m_device, 1, &m_in_flight_fences[m_current_frame]);

	if (vkQueueSubmit(m_graphics_queue, 1, &submit_info, m_in_flight_fences[m_current_frame]) != VK_SUCCESS)
	{
		std::cout << "failed to submit draw command buffer!\n";

		cleanup_vulkan();
		m_successfully_initialized = false;
		return;
	}

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swapchains[] = { m_swapchain };
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &image_index;
	present_info.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(m_present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebuffer_resized)
	{
		m_framebuffer_resized = false;
		recreate_swapchain();
	}
	else if (result != VK_SUCCESS)
	{
		std::cout << "failed to present swap chain image!\n";

		cleanup_vulkan();
		m_successfully_initialized = false;
		return;
	}

	m_current_frame = (m_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool impuls::vulkan_renderer_system::create_window()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	m_window = glfwCreateWindow(width, height, "impuls engine", nullptr, nullptr);
	glfwSetWindowUserPointer(m_window, this);
	glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);
	return m_window != nullptr;
}

bool impuls::vulkan_renderer_system::create_instance()
{
	if (enable_validation_layers && !check_validation_layer_support())
	{
		std::cout << "validation layers requested, but not available!\n";
		return false;
	}

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "test";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "impuls engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	auto extensions = get_required_extensions();
	create_info.enabledExtensionCount = static_cast<ui32>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	if (enable_validation_layers)
	{
		create_info.enabledLayerCount = static_cast<ui32>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	}
	else
	{
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&create_info, nullptr, &m_instance) != VK_SUCCESS)
	{
		std::cout << "failed to create instance!\n";
		return false;
	}

	return true;
}

bool impuls::vulkan_renderer_system::create_debug_messenger()
{
	if (!enable_validation_layers)
	{
		std::cout << "failed to set up debug messenger! validation layers were not enabled.\n";
		return false;
	}

	VkDebugUtilsMessengerCreateInfoEXT create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = vulkan_debug_callback;
	create_info.pUserData = nullptr;

	if (CreateDebugUtilsMessengerEXT(m_instance, &create_info, nullptr, &m_debug_messenger) != VK_SUCCESS)
	{
		std::cout << "failed to set up debug messenger!\n";
		return false;
	}

	return true;
}

bool impuls::vulkan_renderer_system::create_surface()
{
	if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
	{
		std::cout << "failed to create window surface!\n";
		return false;
	}

	return true;
}

bool impuls::vulkan_renderer_system::create_physical_device()
{
	ui32 device_count = 0;
	vkEnumeratePhysicalDevices(m_instance, &device_count, nullptr);

	if (device_count == 0)
	{
		std::cout << "failed to find GPUs with Vulkan support!\n";
		return false;
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(m_instance, &device_count, devices.data());

	VkPhysicalDevice best_physical_device = VK_NULL_HANDLE;
	ui32 best_score = 0;

	for (const auto& device : devices)
	{
		const ui32 score = physical_device_score(device);
		if (score > best_score)
		{
			best_physical_device = device;
			best_score = score;
		}
	}

	m_physical_device = best_physical_device;

	if (m_physical_device == VK_NULL_HANDLE)
	{
		std::cout << "failed to find a suitable GPU!\n";
		return false;
	}

	return true;
}

bool impuls::vulkan_renderer_system::create_logical_device()
{
	const queue_family_indices indices = find_queue_families(m_physical_device);
	const float queue_priority = 1.0f;

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<ui32> unique_queue_families = { indices.m_graphics_family.value(), indices.m_present_family.value() };

	for (ui32 queueFamily : unique_queue_families)
	{
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queueFamily;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos.push_back(queue_create_info);
	}

	VkPhysicalDeviceFeatures device_features = {};

	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.queueCreateInfoCount = static_cast<ui32>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();
	create_info.pEnabledFeatures = &device_features;

	create_info.enabledExtensionCount = static_cast<ui32>(device_extensions.size());
	create_info.ppEnabledExtensionNames = device_extensions.data();

	if (enable_validation_layers)
	{
		create_info.enabledLayerCount = static_cast<ui32>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	}
	else
	{
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(m_physical_device, &create_info, nullptr, &m_device) != VK_SUCCESS)
	{
		std::cout << "failed to create logical device!\n";
		return false;
	}

	vkGetDeviceQueue(m_device, indices.m_graphics_family.value(), 0, &m_graphics_queue);
	vkGetDeviceQueue(m_device, indices.m_present_family.value(), 0, &m_present_queue);

	return true;
}

bool impuls::vulkan_renderer_system::create_swapchain()
{
	swapchain_support_details swapchain_support = query_swapchain_support(m_physical_device);

	VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.m_formats);
	VkPresentModeKHR present_mode = choose_swap_present_mode(swapchain_support.m_present_modes);
	VkExtent2D extent = choose_swap_extent(swapchain_support.m_capabilities);
	
	ui32 image_count = swapchain_support.m_capabilities.minImageCount + 1;

	if (swapchain_support.m_capabilities.maxImageCount > 0 && image_count > swapchain_support.m_capabilities.maxImageCount)
		image_count = swapchain_support.m_capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = m_surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	const queue_family_indices indices = find_queue_families(m_physical_device);
	ui32 queue_family_indices[] = { indices.m_graphics_family.value(), indices.m_present_family.value() };

	if (indices.m_graphics_family != indices.m_present_family)
	{
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	}
	else
	{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0; // Optional
		create_info.pQueueFamilyIndices = nullptr; // Optional
	}

	create_info.preTransform = swapchain_support.m_capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(m_device, &create_info, nullptr, &m_swapchain) != VK_SUCCESS)
	{
		std::cout << "failed to create swapchain!\n";
		return false;
	}

	vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, nullptr);
	m_swapchain_images.resize(image_count);
	vkGetSwapchainImagesKHR(m_device, m_swapchain, &image_count, m_swapchain_images.data());

	m_swapchain_image_format = surface_format.format;
	m_swapchain_extent = extent;

	return true;
}

bool impuls::vulkan_renderer_system::create_image_views()
{
	m_swapchain_image_views.resize(m_swapchain_images.size());

	for (ui32 i = 0; i < m_swapchain_image_views.size(); i++)
	{
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = m_swapchain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = m_swapchain_image_format;
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_device, &create_info, nullptr, &m_swapchain_image_views[i]) != VK_SUCCESS)
		{
			std::cout << "failed to create swapchain image views!\n";
			return false;
		}
	}

	return true;
}

bool impuls::vulkan_renderer_system::create_render_pass()
{
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = m_swapchain_image_format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	render_pass_info.dependencyCount = 1;
	render_pass_info.pDependencies = &dependency;

	if (vkCreateRenderPass(m_device, &render_pass_info, nullptr, &m_render_pass) != VK_SUCCESS)
	{
		std::cout << "failed to create render pass!\n";
		return false;
	}



	return true;
}

bool impuls::vulkan_renderer_system::create_graphics_pipeline()
{
	const auto vert_shader_code = read_file("shaders/vert.spv");
	const auto frag_shader_code = read_file("shaders/frag.spv");

	VkShaderModule vert_shader = create_shader_module(vert_shader_code);
	if (vert_shader == VK_NULL_HANDLE)
	{
		std::cout << "failed to create vertex shader!\n";
		return false;
	}

	VkShaderModule frag_shader = create_shader_module(frag_shader_code);
	if (frag_shader == VK_NULL_HANDLE)
	{
		std::cout << "failed to create fragment shader!\n";

		vkDestroyShaderModule(m_device, vert_shader, nullptr);
		return false;
	}

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
	vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vert_shader_stage_info.module = vert_shader;
	vert_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
	frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	frag_shader_stage_info.module = frag_shader;
	frag_shader_stage_info.pName = "main";

	VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_info, frag_shader_stage_info };

	auto binding_description = vertex::get_binding_description();
	auto attribute_descriptions = vertex::get_attribute_descriptions();

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = static_cast<ui32>(attribute_descriptions.size());
	vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_swapchain_extent.width;
	viewport.height = (float)m_swapchain_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = m_swapchain_extent;

	VkPipelineViewportStateCreateInfo viewport_state = {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo color_blending = {};
	color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable = VK_FALSE;
	color_blending.logicOp = VK_LOGIC_OP_COPY; // Optional
	color_blending.attachmentCount = 1;
	color_blending.pAttachments = &color_blend_attachment;
	color_blending.blendConstants[0] = 0.0f; // Optional
	color_blending.blendConstants[1] = 0.0f; // Optional
	color_blending.blendConstants[2] = 0.0f; // Optional
	color_blending.blendConstants[3] = 0.0f; // Optional

	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 0; // Optional
	pipeline_layout_info.pSetLayouts = nullptr; // Optional
	pipeline_layout_info.pushConstantRangeCount = 0; // Optional
	pipeline_layout_info.pPushConstantRanges = nullptr; // Optional

	if (vkCreatePipelineLayout(m_device, &pipeline_layout_info, nullptr, &m_pipeline_layout) != VK_SUCCESS)
	{
		std::cout << "failed to create pipeline layout!\n";

		vkDestroyShaderModule(m_device, frag_shader, nullptr);
		vkDestroyShaderModule(m_device, vert_shader, nullptr);
		return false;
	}

	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = 2;
	pipeline_info.pStages = shader_stages;

	pipeline_info.pVertexInputState = &vertex_input_info;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.pRasterizationState = &rasterizer;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pDepthStencilState = nullptr; // Optional
	pipeline_info.pColorBlendState = &color_blending;
	pipeline_info.pDynamicState = nullptr; // Optional

	pipeline_info.layout = m_pipeline_layout;
	pipeline_info.renderPass = m_render_pass;
	pipeline_info.subpass = 0;

	pipeline_info.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipeline_info.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &m_graphics_pipeline) != VK_SUCCESS)
	{
		std::cout << "failed to create graphics pipeline!\n";

		vkDestroyShaderModule(m_device, frag_shader, nullptr);
		vkDestroyShaderModule(m_device, vert_shader, nullptr);
		return false;
	}



	vkDestroyShaderModule(m_device, frag_shader, nullptr);
	vkDestroyShaderModule(m_device, vert_shader, nullptr);

	return true;
}

bool impuls::vulkan_renderer_system::create_framebuffers()
{
	m_swapchain_framebuffers.resize(m_swapchain_image_views.size());

	for (ui16 i = 0; i < m_swapchain_image_views.size(); i++)
	{
		VkImageView attachments[] = { m_swapchain_image_views[i] };

		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = m_render_pass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = attachments;
		framebuffer_info.width = m_swapchain_extent.width;
		framebuffer_info.height = m_swapchain_extent.height;
		framebuffer_info.layers = 1;

		if (vkCreateFramebuffer(m_device, &framebuffer_info, nullptr, &m_swapchain_framebuffers[i]) != VK_SUCCESS)
		{
			std::cout << "failed to create framebuffer!\n";
			return false;
		}
	}

	return true;
}

bool impuls::vulkan_renderer_system::create_command_pool()
{
	const queue_family_indices indices = find_queue_families(m_physical_device);

	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = indices.m_graphics_family.value();
	pool_info.flags = 0; // Optional

	if (vkCreateCommandPool(m_device, &pool_info, nullptr, &m_command_pool) != VK_SUCCESS)
	{
		std::cout << "failed to create command pool!\n";
		return false;
	}

	return true;
}

bool impuls::vulkan_renderer_system::create_vertex_buffer()
{
	VkBufferCreateInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.size = sizeof(vertices[0]) * vertices.size();
	buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_device, &buffer_info, nullptr, &m_vertex_buffer) != VK_SUCCESS)
	{
		std::cout << "failed to create vertex buffer!\n";
		return false;
	}

	return true;
}

bool impuls::vulkan_renderer_system::create_command_buffers()
{
	m_command_buffers.resize(m_swapchain_framebuffers.size());

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = m_command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = (ui32)m_command_buffers.size();

	if (vkAllocateCommandBuffers(m_device, &alloc_info, m_command_buffers.data()) != VK_SUCCESS)
	{
		std::cout << "failed to allocate command buffers!\n";
		return false;
	}

	for (ui16 i = 0; i < m_command_buffers.size(); i++)
	{
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		begin_info.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(m_command_buffers[i], &begin_info) != VK_SUCCESS)
		{
			std::cout << "failed to begin recording command buffer!\n";
			return false;
		}

		VkRenderPassBeginInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = m_render_pass;
		render_pass_info.framebuffer = m_swapchain_framebuffers[i];
		render_pass_info.renderArea.offset = { 0, 0 };
		render_pass_info.renderArea.extent = m_swapchain_extent;
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		render_pass_info.clearValueCount = 1;
		render_pass_info.pClearValues = &clearColor;

		vkCmdBeginRenderPass(m_command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
		
		vkCmdBindPipeline(m_command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics_pipeline);
		vkCmdDraw(m_command_buffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(m_command_buffers[i]);

		if (vkEndCommandBuffer(m_command_buffers[i]) != VK_SUCCESS)
		{
			std::cout << "failed to record command buffer!\n";
			return false;
		}
	}

	return true;
}

bool impuls::vulkan_renderer_system::create_syncers()
{
	m_image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (ui16 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_image_available_semaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_device, &semaphore_info, nullptr, &m_render_finished_semaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_device, &fence_info, nullptr, &m_in_flight_fences[i]) != VK_SUCCESS)
		{
			std::cout << "failed to create syncers!\n";
			return false;
		}
	}

	return true;
}

bool impuls::vulkan_renderer_system::recreate_swapchain()
{
	int loc_width = 0, loc_height = 0;
	while (loc_width == 0 || loc_height == 0)
	{
		glfwGetFramebufferSize(m_window, &loc_width, &loc_height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(m_device);

	cleanup_swapchain();

	if (!create_swapchain() ||
		!create_image_views() ||
		!create_render_pass() ||
		!create_graphics_pipeline() ||
		!create_framebuffers() ||
		!create_command_buffers())
	{
		std::cout << "failed to recreate swapchain!\n";
		return false;
	}

	return true;
}

void impuls::vulkan_renderer_system::cleanup_swapchain()
{
	for (auto framebuffer : m_swapchain_framebuffers)
		vkDestroyFramebuffer(m_device, framebuffer, nullptr);

	vkFreeCommandBuffers(m_device, m_command_pool, static_cast<ui32>(m_command_buffers.size()), m_command_buffers.data());

	vkDestroyPipeline(m_device, m_graphics_pipeline, nullptr);
	vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);
	vkDestroyRenderPass(m_device, m_render_pass, nullptr);

	for (auto& image_view : m_swapchain_image_views)
		vkDestroyImageView(m_device, image_view, nullptr);

	vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

void impuls::vulkan_renderer_system::cleanup_vulkan()
{
	cleanup_swapchain();

	vkDestroyBuffer(m_device, m_vertex_buffer, nullptr);

	for (ui16 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_device, m_render_finished_semaphores[i], nullptr);
		vkDestroySemaphore(m_device, m_image_available_semaphores[i], nullptr);
		vkDestroyFence(m_device, m_in_flight_fences[i], nullptr);
	}

	vkDestroyCommandPool(m_device, m_command_pool, nullptr);

	vkDestroyDevice(m_device, nullptr);

	if (enable_validation_layers)
		DestroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

	vkDestroyInstance(m_instance, nullptr);

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

bool impuls::vulkan_renderer_system::check_validation_layer_support() const
{
	ui32 layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const char* layer_name : validation_layers)
	{
		bool layer_found = false;

		for (const auto& layer_properties : available_layers)
		{
			if (strcmp(layer_name, layer_properties.layerName) == 0)
			{
				layer_found = true;
				break;
			}
		}

		if (!layer_found)
			return false;
	}

	return true;
}

bool impuls::vulkan_renderer_system::check_physical_device_extension_support(VkPhysicalDevice in_device) const
{
	ui32 extension_count;
	vkEnumerateDeviceExtensionProperties(in_device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(in_device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

	for (const auto& extension : available_extensions)
		required_extensions.erase(extension.extensionName);

	return required_extensions.empty();
}

std::vector<const char*> impuls::vulkan_renderer_system::get_required_extensions() const
{
	ui32 glfw_extension_count = 0;
	const char** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if (enable_validation_layers)
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

impuls::ui32 impuls::vulkan_renderer_system::physical_device_score(VkPhysicalDevice in_device) const
{
	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceProperties(in_device, &device_properties);
	vkGetPhysicalDeviceFeatures(in_device, &device_features);

	// Application can't function without geometry shaders
	if (!device_features.geometryShader)
		return 0;

	//we need a queue that handles graphics
	if (!find_queue_families(in_device).is_complete())
		return 0;

	if (!check_physical_device_extension_support(in_device))
		return 0;

	swapchain_support_details swapchain_support = query_swapchain_support(in_device);
	if (swapchain_support.m_formats.empty() || swapchain_support.m_present_modes.empty())
		return 0;

	ui32 score = 0;

	// Discrete GPUs have a significant performance advantage
	if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		score += 1000;

	// Maximum possible size of textures affects graphics quality
	score += device_properties.limits.maxImageDimension2D;

	return score;
}

impuls::queue_family_indices impuls::vulkan_renderer_system::find_queue_families(VkPhysicalDevice in_device) const
{
	queue_family_indices indices;

	ui32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(in_device, &queue_family_count, nullptr);

	std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(in_device, &queue_family_count, queue_families.data());

	for (ui32 i = 0; i < queue_family_count; i++)
	{
		if (queue_families[i].queueCount > 0 && queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indices.m_graphics_family = i;

		VkBool32 present_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(in_device, i, m_surface, &present_support);

		if (queue_families[i].queueCount > 0 && present_support)
			indices.m_present_family = i;

		if (indices.is_complete())
			break;
	}

	return indices;
}

impuls::swapchain_support_details impuls::vulkan_renderer_system::query_swapchain_support(VkPhysicalDevice in_device) const
{
	swapchain_support_details details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(in_device, m_surface, &details.m_capabilities);

	ui32 format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(in_device, m_surface, &format_count, nullptr);

	if (format_count != 0)
	{
		details.m_formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(in_device, m_surface, &format_count, details.m_formats.data());
	}

	ui32 present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(in_device, m_surface, &present_mode_count, nullptr);

	if (present_mode_count != 0)
	{
		details.m_present_modes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(in_device, m_surface, &present_mode_count, details.m_present_modes.data());
	}

	return details;
}

VkSurfaceFormatKHR impuls::vulkan_renderer_system::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& in_available_formats) const
{
	if (in_available_formats.size() == 1 && in_available_formats[0].format == VK_FORMAT_UNDEFINED)
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	for (const auto& available_format : in_available_formats)
		if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return available_format;

	return in_available_formats[0];
}

VkPresentModeKHR impuls::vulkan_renderer_system::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& in_available_present_modes) const
{
	VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& available_present_mode : in_available_present_modes)
	{
		if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return available_present_mode;
		else if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			best_mode = available_present_mode;
	}

	return best_mode;
}

VkExtent2D impuls::vulkan_renderer_system::choose_swap_extent(const VkSurfaceCapabilitiesKHR& in_capabilities) const
{
	if (in_capabilities.currentExtent.width != std::numeric_limits<ui32>::max())
		return in_capabilities.currentExtent;
	
	int cur_width, cur_height;
	glfwGetFramebufferSize(m_window, &cur_width, &cur_height);

	VkExtent2D actual_extent = { static_cast<ui32>(cur_width), static_cast<ui32>(cur_height) };

	actual_extent.width = std::clamp(actual_extent.width, in_capabilities.minImageExtent.width, in_capabilities.maxImageExtent.width);
	actual_extent.height = std::clamp(actual_extent.height, in_capabilities.minImageExtent.height, in_capabilities.maxImageExtent.height);

	return actual_extent;
}

VkShaderModule impuls::vulkan_renderer_system::create_shader_module(const std::vector<char>& in_code)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = in_code.size();
	create_info.pCode = reinterpret_cast<const ui32*>(in_code.data());

	VkShaderModule shader_module;

	if (vkCreateShaderModule(m_device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
	{
		return VK_NULL_HANDLE;
	}

	return shader_module;
}
