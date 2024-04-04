#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan_core.h>

namespace flwfrg
{
class VulkanContext;
class VulkanRenderpass;

class VulkanFrameBuffer
{
public:
	VulkanFrameBuffer(VulkanContext *context, VulkanRenderpass &renderpass, uint32_t width, uint32_t height, std::vector<VkImageView> attachments);
	VulkanFrameBuffer(VulkanFrameBuffer &&other) noexcept;
	~VulkanFrameBuffer();

	// Not copyable or movable
	VulkanFrameBuffer(const VulkanFrameBuffer &) = delete;
	VulkanFrameBuffer &operator=(const VulkanFrameBuffer &) = delete;

	// Methods

	[[nodiscard]] VkFramebuffer get_handle() noexcept { return handle_; };

private:
	VulkanContext *context_;

	VkFramebuffer handle_;
	VulkanRenderpass &renderpass_;
	std::vector<VkImageView> attachments_;
};

}// namespace flwfrg