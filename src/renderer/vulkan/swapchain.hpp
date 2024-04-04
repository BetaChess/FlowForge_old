#pragma once

#include <memory>

#include "device.hpp"
#include "frame_buffer.hpp"
#include "image.hpp"

namespace flwfrg
{

class VulkanSwapchain
{
public:
	explicit VulkanSwapchain(VulkanContext *context);
	~VulkanSwapchain();

	// Not copyable or movable
	VulkanSwapchain(const VulkanSwapchain &) = delete;
	VulkanSwapchain &operator=(const VulkanSwapchain &) = delete;
	VulkanSwapchain(VulkanSwapchain &&) = delete;
	VulkanSwapchain &operator=(VulkanSwapchain &&) = delete;

	// Methods
	bool acquire_next_image(uint64_t timeout_ns, VkSemaphore image_availiable_semaphore, VkFence fence, uint32_t *out_image_index);
	bool present(VkQueue graphics_queue, VkQueue present_queue, VkSemaphore render_complete_semaphore, uint32_t present_image_index);
	[[nodiscard]] inline uint8_t get_image_count() const { return swapchain_images_.size(); };

private:
	VulkanContext *context_;

	VkSurfaceFormatKHR swapchain_image_format_;
	uint8_t max_frames_in_flight_ = 2;
	VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;

	std::vector<VkImage> swapchain_images_;
	std::vector<VkImageView> swapchain_image_views_;
	std::unique_ptr<VulkanImage> depth_attachment_;

	std::vector<VulkanFrameBuffer> frame_buffers_;

	void recreate_swapchain();

	bool choose_swapchain_surface_format();

	friend VulkanContext;
	friend VulkanRenderpass;
};


}// namespace flwfrg