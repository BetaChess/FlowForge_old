#include "pch.hpp"

#include "vulkan_context.hpp"
#include "imgui_impl_vulkan.h"

#include <unordered_set>



namespace flwfrg
{

///// Local helper functions

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
		void *pUserData)
{
	
	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		FLOWFORGE_ERROR("VK_VALIDATION {}", pCallbackData->pMessage);
	} else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		FLOWFORGE_WARN("VK_VALIDATION {}", pCallbackData->pMessage);
	} else
	{
		FLOWFORGE_TRACE("VK_VALIDATION {}", pCallbackData->pMessage);
	}
	
	return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(
		VkInstance instance_,
		const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
		const VkAllocationCallbacks *pAllocator,
		VkDebugUtilsMessengerEXT *pDebugMessenger)
{
	
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
			instance_,
			"vkCreateDebugUtilsMessengerEXT");
	
	if (func != nullptr)
	{
		return func(instance_, pCreateInfo, pAllocator, pDebugMessenger);
	} else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(
		VkInstance instance_,
		VkDebugUtilsMessengerEXT debugMessenger,
		const VkAllocationCallbacks *pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
			instance_,
			"vkDestroyDebugUtilsMessengerEXT");
	
	if (func != nullptr)
	{
		func(instance_, debugMessenger, pAllocator);
	}
	
}


///// Method implementations

VulkanContext::VulkanContext(Window& window)
	: window_{window}
{
	window_.register_resize_callback(resize_callback, this);
	
	FLOWFORGE_INFO("Creating frame buffers");
	regenerate_framebuffers();
	FLOWFORGE_INFO("Creating command buffers");
	create_command_buffers();

	image_avaliable_semaphores_.resize(swapchain_.max_frames_in_flight_);
	queue_complete_semaphores_.resize(swapchain_.max_frames_in_flight_);
	for (size_t i = 0; i < swapchain_.max_frames_in_flight_; i++)
	{
		VkSemaphoreCreateInfo semaphore_info{};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(device_.logical_device_, &semaphore_info, nullptr, &image_avaliable_semaphores_[i]);
		vkCreateSemaphore(device_.logical_device_, &semaphore_info, nullptr, &queue_complete_semaphores_[i]);
		
		in_flight_fences_.emplace_back(this, true);
	}

	images_in_flight_.resize(swapchain_.get_image_count());

	imgui_instance_ = ImGuiInstance(this);
}
VulkanContext::~VulkanContext()
{
	vkDeviceWaitIdle(device_.logical_device_);

	// Destroy semaphores
	for (size_t i = 0; i < swapchain_.max_frames_in_flight_; i++)
	{
		vkDestroySemaphore(device_.logical_device_, image_avaliable_semaphores_[i], nullptr);
		vkDestroySemaphore(device_.logical_device_, queue_complete_semaphores_[i], nullptr);
	}
}

void VulkanContext::populate_imgui_init_info(ImGui_ImplVulkan_InitInfo &out_init_info)
{
	out_init_info.Instance = instance_;
	out_init_info.PhysicalDevice = device_.physical_device_;
	out_init_info.Device = device_.logical_device_;
	out_init_info.QueueFamily = device_.graphics_queue_index_;
	out_init_info.Queue = device_.graphics_queue_;
	out_init_info.PipelineCache = VK_NULL_HANDLE;
	out_init_info.DescriptorPool = object_shader_.get_global_descriptor_pool().get();
	out_init_info.Subpass = 0;
	out_init_info.MinImageCount = swapchain_.get_image_count();
	out_init_info.ImageCount = swapchain_.get_image_count();
	out_init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	out_init_info.Allocator = nullptr;
	out_init_info.CheckVkResultFn = nullptr;
	out_init_info.RenderPass = get_main_render_pass();
}

int32_t VulkanContext::find_memory_index(uint32_t type_filter, VkMemoryPropertyFlags memory_flags)
{
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(device_.physical_device_, &memory_properties);

	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
	{
		if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & memory_flags) == memory_flags)
		{
			return static_cast<int32_t>(i);
		}
	}

	FLOWFORGE_WARN("Unable to find suitable memory type");
	return -1;
}

void VulkanContext::create_command_buffers()
{
	graphics_command_buffers_.clear();
	
	for (size_t i = 0; i < swapchain_.get_image_count(); i++)
	{
		graphics_command_buffers_.emplace_back(this, device_.graphics_command_pool_, true);
	}
}

void VulkanContext::regenerate_framebuffers()
{
	swapchain_.frame_buffers_.clear();
	
	for (size_t i = 0; i < swapchain_.get_image_count(); i++)
	{
		std::vector<VkImageView> attachments{swapchain_.swapchain_image_views_[i], swapchain_.depth_attachment_->view_};
		swapchain_.frame_buffers_.emplace_back(
			this,
			main_renderpass_,
			device_.swapchain_support_.capabilities.currentExtent.width,
			device_.swapchain_support_.capabilities.currentExtent.height,
			attachments);
	}
}
void VulkanContext::resize_callback(void *context)
{
	auto* vulkan_context = reinterpret_cast<VulkanContext*>(context);

	FLOWFORGE_TRACE("Resize callback beginning in vulkan context");

	vulkan_context->swapchain_.recreate_swapchain();
	vulkan_context->regenerate_framebuffers();
}


VulkanInstance::VulkanInstance()
{
	FLOWFORGE_INFO("Creating Vulkan instance");
	
	// Check if the validation layers are enabled and supported.
	if (VulkanContext::enable_validation_layers_ && !validation_layers_supported(validationLayers))
	{
		// If they are enabled but not supported, throw a runtime error.
		throw std::runtime_error("Validation layers requested, but not available! ");
	}
	
	// Create the appinfo struct.
	VkApplicationInfo appInfo{};
	// Specify the struct as a type application info. (a struct used to give information about the instance).
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	// Set the name
	appInfo.pApplicationName = "FlowForge";
	// Specify the application version
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	// A custom engine is used.
	appInfo.pEngineName = "FlowForge";
	// Specify the engine version
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	// Specify the vulkan API version.
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// Create the instance_ create info
	VkInstanceCreateInfo createInfo{};
	// Specify that this struct is the info to create the instance.
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	// give it the application info created earlier.
	createInfo.pApplicationInfo = &appInfo;

	// Get and enable the glfw extensions
	auto extensions = get_required_extensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());// The number of enabled extensions
	createInfo.ppEnabledExtensionNames = extensions.data();                     // The names of the actual extensions

	// create a temporary debugger for creation of the Vulkan instance_
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	// check if validation layers are enabled.
	if constexpr (VulkanContext::enable_validation_layers_)
	{
		// Specify the enabled validation layers
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());// The number of validation layers
		createInfo.ppEnabledLayerNames = validationLayers.data();                     // The names of the validation layers

		// Use a helper function to populate the createInfo for the debugMessenger.
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
										  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
										  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
									  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
									  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;
		debugCreateInfo.pUserData = nullptr;  // Optional
		
		// Give the struct the creation info.
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *) &debugCreateInfo;
	} else
	{
		// Specify that no layers are enabled (0)
		createInfo.enabledLayerCount = 0;

		// Give it a nullptr to the creation info.
		createInfo.pNext = nullptr;
	}
	
	// Finally, create the instance_
	if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS)
	{
		// If the instance failed to be created, throw a runtime error.
		throw std::runtime_error("failed to create instance_!");
	}
	
	check_glfw_required_instance_extensions();
}
VulkanInstance::~VulkanInstance()
{
	vkDestroyInstance(instance_, nullptr);
	FLOWFORGE_INFO("Vulkan instance destroyed");
}

bool VulkanInstance::validation_layers_supported(const std::vector<const char *> &layers)
{
	// Get the validation layer count
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	
	// Create a vector to store the available layers
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	// print the available layers
	{
		std::stringstream ss;

		ss << "\n available layers:";

		for (const auto &layer: availableLayers)
			ss << "\n\t" << layer.layerName;

		FLOWFORGE_TRACE(ss.str());
	}

	// Iterate through each layer name in the list of required layers
	for (const char *layerName: layers)
	{
		bool layerFound = false;

		// Check if the layer is supported by iterating through the supported layers
		for (const auto &layerProperties: availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		// If any layer is not found. Return false.
		if (!layerFound)
		{
			FLOWFORGE_ERROR("Layer was not found during Validation Layer Support checking! Layer name is: {}", layerName);
			return false;
		}
	}

	return true;
}

std::vector<const char *> VulkanInstance::get_required_extensions()
{
	// Get the number of extensions required by glfw.
	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	
	// Put them in the vector of required extensions
	std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	
	// If validation layers are enabled. Throw in the Debug Utils extension.
	if constexpr (VulkanContext::enable_validation_layers_)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	
	// Return the vector.
	return extensions;
}

void VulkanInstance::check_glfw_required_instance_extensions()
{
	// Create a variable to store the number of supported extensions
	uint32_t extensionCount = 0;
	
	// Get the number of avaliable extensions. (The middle parameter of the function writes this to the specified memory location)
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	// Change the size of the extensions vetor to match the number of avaliable extensions.
	std::vector<VkExtensionProperties> extensions(extensionCount);
	// Write the extensions to the vector. 
	// (the last parameter is a memory location to the place the extension properties should be written, 
	//		in this case, the data location of the vector)
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
	
	// List out the avaliable extensions and put the names in a string data structure.
	std::unordered_set<std::string> available;
	{
		std::stringstream ss;
		ss << "\n available extensions:";
		for (const auto &extension: extensions)
		{
			ss << "\n\t" << extension.extensionName;
			available.insert(extension.extensionName);
		}
		
		FLOWFORGE_TRACE(ss.str());
	}
	
	{
		std::stringstream ss;
		// List out the required extensions.
		ss << "\n required extensions:";
		// Get the required extensions.
		auto requiredExtensions = get_required_extensions();
		// Iterate through the required extensions.
		for (const auto &required: requiredExtensions)
		{
			ss << "\n\t" << required;
			// If the required extension is not available, throw a runtime error.
			if (available.find(required) == available.end())
			{
				FLOWFORGE_FATAL("Missing required glfw extension ({})", required);
				throw std::runtime_error("Missing required glfw extension");
			}
		}
		
		FLOWFORGE_TRACE(ss.str());
	}
}


VulkanDebugMessenger::VulkanDebugMessenger(const VulkanInstance& instance)
	: instance_{instance}
{
	FLOWFORGE_INFO("Setting Vulkan debug messenger");
	
	// Create the debug messenger create info.
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
								 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
							 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr;  // Optional
	
	// Try to create the debug messenger. Throw a runtime error if it failed.
	if (CreateDebugUtilsMessengerEXT(instance_, &createInfo, nullptr, &debug_messenger_) != VK_SUCCESS)
	{
		FLOWFORGE_FATAL("Failed to set up debug messenger");
		throw std::runtime_error("Failed to set up debug messenger!");
	}
}
VulkanDebugMessenger::~VulkanDebugMessenger()
{
	DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
	FLOWFORGE_INFO("Vulkan debug callback destroyed");
}
VulkanSurface::VulkanSurface(const VulkanInstance &instance, const Window &window)
	: instance_{instance}, window_{window}, surface_{window.create_window_surface(instance)}
{
}
VulkanSurface::~VulkanSurface()
{
	vkDestroySurfaceKHR(instance_, surface_, nullptr);
	FLOWFORGE_INFO("Vulkan surface destroyed");
}

}// namespace flwfrg