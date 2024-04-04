#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace aito
{
class VulkanContext;

class VulkanDescriptorPool
{
public:
	inline VulkanDescriptorPool() = default;
	VulkanDescriptorPool(VulkanContext *context, VkDescriptorPoolCreateInfo create_info);
	~VulkanDescriptorPool();

	// Not copyable but movable
	VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
	VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;
	VulkanDescriptorPool(VulkanDescriptorPool&& other) noexcept;
	VulkanDescriptorPool& operator=(VulkanDescriptorPool&& other) noexcept;

	[[nodiscard]] VkDescriptorPool get() const { return pool_; }
	
private:
	VulkanContext *context_ = nullptr;
	
	VkDescriptorPool pool_ = VK_NULL_HANDLE;
};

class VulkanDescriptorSetLayout
{
public:
	inline VulkanDescriptorSetLayout() = default;
	VulkanDescriptorSetLayout(VulkanContext *context, VkDescriptorSetLayoutCreateInfo create_info);
	~VulkanDescriptorSetLayout();

	// Not copyable but movable
	VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
	VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;
	VulkanDescriptorSetLayout(VulkanDescriptorSetLayout&& other) noexcept;
	VulkanDescriptorSetLayout& operator=(VulkanDescriptorSetLayout&& other) noexcept;

	[[nodiscard]] VkDescriptorSetLayout get() const { return layout_; }
	
private:
	VulkanContext *context_ = nullptr;
	
	VkDescriptorSetLayout layout_ = VK_NULL_HANDLE;
};

}// namespace aito