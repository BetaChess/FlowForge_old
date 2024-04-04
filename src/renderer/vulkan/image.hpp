#pragma once

#include "device.hpp"
#include <vulkan/vulkan_core.h>


namespace aito
{

class VulkanContext;
class VulkanCommandBuffer;
class VulkanBuffer;

class VulkanImage
{
public:
	VulkanImage() = default;
	VulkanImage(
			VulkanContext *context,
			uint32_t width,
			uint32_t height,
			VkFormat format,
			VkImageTiling tiling,
			VkImageUsageFlags usage,
			VkMemoryPropertyFlags memory_flags,
			VkImageAspectFlags aspect_flags,
			bool create_view = true);
	~VulkanImage();

	// Not copyable but movable
	VulkanImage(const VulkanImage &) = delete;
	VulkanImage &operator=(const VulkanImage &) = delete;
	VulkanImage(VulkanImage &&other) noexcept;
	VulkanImage &operator=(VulkanImage &&other) noexcept = default;

	void transition_layout(VulkanCommandBuffer &command_buffer,
						  VkFormat format,
						  VkImageLayout old_layout,
						  VkImageLayout new_layout);

	void copy_from_buffer(VulkanCommandBuffer& command_buffer, VulkanBuffer& buffer);

private:
	VulkanContext *context_ = nullptr;

	VkImage image_handle_ = VK_NULL_HANDLE;
	VkDeviceMemory memory_ = VK_NULL_HANDLE;
	VkImageView view_ = VK_NULL_HANDLE;
	uint32_t width_ = 0;
	uint32_t height_ = 0;

	void view_create(VkFormat format, VkImageAspectFlags aspect_flags);

	friend VulkanContext;
};

}// namespace aito