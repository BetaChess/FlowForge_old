#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace aito
{
class VulkanCommandBuffer;
class VulkanRenderpass;
class VulkanContext;
class VulkanSwapchain;
class VulkanImage;
class VulkanFrameBuffer;
class VulkanFence;
class VulkanRenderer;

struct VulkanPhysicalDeviceRequirements {
	bool graphics = true;
	bool present = true;
	bool compute = false;
	bool transfer = true;
	std::vector<const char *> device_extension_names{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	bool sampler_anisotropy = true;
	bool discrete_gpu = false;
};

struct SwapchainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> present_modes;
};

/// <summary>
/// Struct for storing the indices of queue families.
/// </summary>
struct QueueFamilyIndices {
	std::optional<uint32_t> graphics_family_index;
	std::optional<uint32_t> present_family_index;
	std::optional<uint32_t> compute_family_index;
	std::optional<uint32_t> transfer_family_index;

	[[nodiscard]] constexpr bool is_complete() const
	{
		return graphics_family_index.has_value() && present_family_index.has_value();
	}
};


class VulkanDevice
{
public:
	explicit VulkanDevice(VulkanContext *vulkanContext);
	~VulkanDevice();

	[[nodiscard]] VkQueue get_present_queue() const { return present_queue_; };
	[[nodiscard]] VkQueue get_graphics_queue() const { return graphics_queue_; };
	[[nodiscard]] VkQueue get_transfer_queue() const { return transfer_queue_; };

	[[nodiscard]] uint32_t get_graphics_queue_index() const { return graphics_queue_index_; };
	[[nodiscard]] uint32_t get_present_queue_index() const { return present_queue_index_; };
	[[nodiscard]] uint32_t get_transfer_queue_index() const { return transfer_queue_index_; };
	
	[[nodiscard]] VkDevice get_logical_device() const { return logical_device_; };
	
	[[nodiscard]] VkPhysicalDevice get_physical_device() const { return physical_device_; };
	[[nodiscard]] VkCommandPool get_graphics_command_pool() const { return graphics_command_pool_; };
	[[nodiscard]] VkPhysicalDeviceProperties get_physical_device_properties() const { return physical_device_properties_; };

	
private:
#ifdef NDEBUG
	const bool enable_validation_layers_ = false;
#else
	const bool enable_validation_layers_ = true;
#endif

	VulkanContext *vulkan_context_;
	VulkanPhysicalDeviceRequirements physical_device_requirements_{};

	VkPhysicalDevice physical_device_;
	VkDevice logical_device_;
	SwapchainSupportDetails swapchain_support_;
	
	uint32_t graphics_queue_index_;
	uint32_t present_queue_index_;
	uint32_t transfer_queue_index_;
	VkQueue graphics_queue_;
	VkQueue present_queue_;
	VkQueue transfer_queue_;

	VkCommandPool graphics_command_pool_;

	VkPhysicalDeviceProperties physical_device_properties_;
	VkPhysicalDeviceFeatures features_;
	VkPhysicalDeviceMemoryProperties memory_;

	VkFormat depth_format_ = VK_FORMAT_UNDEFINED;


	///// Private methods

	void pick_physical_device();

	void create_logical_device();

	///// Helper methods

	[[nodiscard]] bool is_device_suitable(VkPhysicalDevice device);

	uint32_t rate_device_suitability(VkPhysicalDevice device);

	QueueFamilyIndices find_queue_families(VkPhysicalDevice device);

	SwapchainSupportDetails query_swapchain_support(VkPhysicalDevice device);

	inline SwapchainSupportDetails query_swapchain_support() { return query_swapchain_support(physical_device_); };
	
	[[nodiscard]] bool check_device_extension_support(VkPhysicalDevice device);

	bool detect_depth_format();

	friend VulkanContext;
	friend VulkanSwapchain;
	friend VulkanImage;
	friend VulkanRenderpass;
	friend VulkanCommandBuffer;
	friend VulkanFrameBuffer;
	friend VulkanFence;
	friend VulkanRenderer;
};

}// namespace aito