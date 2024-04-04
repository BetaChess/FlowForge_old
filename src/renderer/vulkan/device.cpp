#include "pch.hpp"

#include "device.hpp"
#include "vulkan_context.hpp"

#include <map>
#include <set>

namespace flwfrg
{

VulkanDevice::VulkanDevice(VulkanContext *context)
	: vulkan_context_{context}
{
	assert(context != nullptr);
	
	pick_physical_device();
	create_logical_device();
}

VulkanDevice::~VulkanDevice()
{
	vkDestroyCommandPool(logical_device_, graphics_command_pool_, nullptr);
	FLOWFORGE_INFO("Graphics command pool destroyed");
	vkDestroyDevice(logical_device_, nullptr);
	FLOWFORGE_INFO("Logical device destroyed");
}

void VulkanDevice::pick_physical_device()
{
	// Find the number of physical devices that support vulkan.
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(vulkan_context_->instance_, &deviceCount, nullptr);

	// Check if any of the graphics cards support vulkan.
	if (deviceCount == 0)
	{
		FLOWFORGE_FATAL("Failed to find GPUs with Vulkan support");
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}

	// Store the physical handles in an array
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(vulkan_context_->instance_, &deviceCount, devices.data());

	// Use an ordered map to automatically sort candidates by increasing score
	std::multimap<uint32_t, VkPhysicalDevice> candidates;

	for (const auto &device: devices)
	{
		uint32_t score = rate_device_suitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	// Get the physical properties
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(candidates.rbegin()->second, &deviceProperties);

	// Print the name of the chosen GPU
	FLOWFORGE_INFO("Best GPU found is: {}", deviceProperties.deviceName);


	// Check if the best candidate is suitable at all
	if (candidates.rbegin()->first > 0)
	{
		physical_device_ = candidates.rbegin()->second;

		auto queue_indices = find_queue_families(physical_device_);
		graphics_queue_index_ = queue_indices.graphics_family_index.value();
		present_queue_index_ = queue_indices.present_family_index.value();
		transfer_queue_index_ = queue_indices.transfer_family_index.value();

		physical_device_properties_ = deviceProperties;
		swapchain_support_ = query_swapchain_support();
	} else
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

uint32_t VulkanDevice::rate_device_suitability(VkPhysicalDevice device)
{
	// First, check if the device is suitable at all.
	if (!is_device_suitable(device))
		return 0;

	// Get the device properties
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Get the device features
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	uint32_t score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	return score;
}

bool VulkanDevice::is_device_suitable(VkPhysicalDevice device)
{
	// The requirements
	VulkanPhysicalDeviceRequirements requirements{};

	// Get the device properties
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Get the device features
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	if (requirements.sampler_anisotropy && deviceFeatures.samplerAnisotropy == VK_FALSE)
		return false;

	// Get the device queue families
	QueueFamilyIndices indices = find_queue_families(device);

	// Check that all required families exist
	if (requirements.compute && !indices.compute_family_index.has_value())
		return false;
	if (requirements.graphics && !indices.graphics_family_index.has_value())
		return false;
	if (requirements.present && !indices.present_family_index.has_value())
		return false;
	if (requirements.transfer && !indices.transfer_family_index.has_value())
		return false;

	// Check for extension support
	bool extensionsSupported = check_device_extension_support(device);

	// Check for swap chain adequacy
	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapchainSupportDetails swapChainSupport = query_swapchain_support(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.present_modes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.is_complete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices VulkanDevice::find_queue_families(VkPhysicalDevice device)
{
	QueueFamilyIndices indices{};
	// Logic to find queue family indices to populate struct with
	// Queue family count
	uint32_t queueFamilyCount = 0;
	// Get the number of queue families.
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	// Get queue family properties
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	// Look at each queue
	uint8_t smallest_queue_count = 255;
	for (size_t i = 0; i < queueFamilyCount; i++)
	{
		uint8_t current_family_count = 0;

		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphics_family_index = i;
			current_family_count++;
		}

		if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			indices.compute_family_index = i;
			current_family_count++;
		}

		if ((queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && current_family_count <= smallest_queue_count)
		{
			indices.transfer_family_index = i;
			smallest_queue_count = current_family_count;
		}

		VkBool32 supports_present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vulkan_context_->surface_, &supports_present);
		if (supports_present)
		{
			indices.present_family_index = i;
		}
	}

	return indices;
}

bool VulkanDevice::check_device_extension_support(VkPhysicalDevice device)
{
	// Get the extension count
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	// Get the available extensions
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	// Make a string set of the required extensions
	std::set<std::string> requiredExtensions(physical_device_requirements_.device_extension_names.begin(), physical_device_requirements_.device_extension_names.end());

	// Erase any supported extensions from the set
	for (const auto &extension: availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	// Check if the set is empty. If it is, all the extensions are supported.
	return requiredExtensions.empty();
}
bool VulkanDevice::detect_depth_format()
{
	// Candidate formats
	std::vector<VkFormat> candidates = {
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT};

	uint32_t flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	for (VkFormat format: candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physical_device_, format, &props);
		if (((props.optimalTilingFeatures & flags) == flags) ||
			((props.linearTilingFeatures & flags) == flags))
		{
			depth_format_ = format;
			return true;
		}
	}

	return false;
}

SwapchainSupportDetails VulkanDevice::query_swapchain_support(VkPhysicalDevice device)
{
	SwapchainSupportDetails details;

	// Get the surface capabilities of the physical device
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vulkan_context_->surface_, &details.capabilities);

	// Get the format count
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkan_context_->surface_, &formatCount, nullptr);

	// if the format count isn't 0, resize the format list and insert the format details.
	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, vulkan_context_->surface_, &formatCount, details.formats.data());
	}

	// Get the present mode count
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkan_context_->surface_, &presentModeCount, nullptr);

	// if the present mode count isn't 0, resize the mode list and populate it.
	if (presentModeCount != 0)
	{
		details.present_modes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, vulkan_context_->surface_, &presentModeCount, details.present_modes.data());
	}

	return details;
}

void VulkanDevice::create_logical_device()
{
	bool present_shares_graphics_queue = graphics_queue_index_ == present_queue_index_;
	bool transfer_shares_graphics_queue = graphics_queue_index_ == transfer_queue_index_;

	size_t index_count = 1;
	if (!present_shares_graphics_queue)
	{
		index_count++;
	}
	if (!transfer_shares_graphics_queue)
	{
		index_count++;
	}

	uint32_t indices[index_count];
	size_t index = 0;
	indices[index++] = graphics_queue_index_;
	if (!present_shares_graphics_queue)
	{
		indices[index++] = present_queue_index_;
	}
	if (!transfer_shares_graphics_queue)
	{
		indices[index++] = transfer_queue_index_;
	}

	VkDeviceQueueCreateInfo queue_create_infos[index_count];
	for (size_t i = 0; i < index_count; ++i)
	{
		queue_create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_infos[i].queueFamilyIndex = indices[i];
		queue_create_infos[i].queueCount = 1;
		//		if (indices[i] == graphics_queue_index_) {
		//			queue_create_infos[i].queueCount = 2;
		//		}
		queue_create_infos[i].flags = 0;
		queue_create_infos[i].pNext = nullptr;
		float queue_priority = 1.0f;
		queue_create_infos[i].pQueuePriorities = &queue_priority;
	}

	// Request device features.
	VkPhysicalDeviceFeatures device_features = {};
	device_features.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo device_create_info = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	device_create_info.queueCreateInfoCount = index_count;
	device_create_info.pQueueCreateInfos = queue_create_infos;
	device_create_info.pEnabledFeatures = &device_features;
	device_create_info.enabledExtensionCount = 1;
	const char *extension_names = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	device_create_info.ppEnabledExtensionNames = &extension_names;

	// Deprecated and ignored
	device_create_info.enabledLayerCount = 0;
	device_create_info.ppEnabledLayerNames = nullptr;

	// Create the device.
	if (vkCreateDevice(
				physical_device_,
				&device_create_info,
				nullptr,
				&logical_device_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create logical device");
	}

	FLOWFORGE_INFO("Logical device created");

	// Get queues.
	vkGetDeviceQueue(
			logical_device_,
			graphics_queue_index_,
			0,
			&graphics_queue_);

	vkGetDeviceQueue(
			logical_device_,
			present_queue_index_,
			0,
			&present_queue_);

	vkGetDeviceQueue(
			logical_device_,
			transfer_queue_index_,
			0,
			&transfer_queue_);

	FLOWFORGE_INFO("Queues obtained");

	// Create the command pool
	VkCommandPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = graphics_queue_index_;
	pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(logical_device_, &pool_info, nullptr, &graphics_command_pool_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool");
	}

	FLOWFORGE_INFO("Graphics command pool created");
}

}// namespace flwfrg
