#pragma once

#include <vulkan/vulkan_core.h>

namespace aito
{
class VulkanContext;
}
namespace aito
{
class VulkanRenderpass;

class VulkanCommandBuffer
{
public:
	enum class State
	{
		READY,
		RECORDING,
		IN_RENDER_PASS,
		RECORDING_ENDED,
		SUBMITTED,
		NOT_ALLOCATED
	};

public:
	VulkanCommandBuffer(VulkanContext *context, VkCommandPool pool, bool is_primary);
	~VulkanCommandBuffer();


	// Not copyable but movable
	VulkanCommandBuffer(const VulkanCommandBuffer &) = delete;
	VulkanCommandBuffer &operator=(const VulkanCommandBuffer &) = delete;
	VulkanCommandBuffer(VulkanCommandBuffer &&other) noexcept;
	VulkanCommandBuffer &operator=(VulkanCommandBuffer &&) = default;

	// Methods
	[[nodiscard]] inline VkCommandBuffer get_handle() noexcept { return handle_; };

	void begin(bool is_single_use, bool is_renderpass_continue, bool is_simultaneous_use);
	void end();
	void submit(VkQueue queue, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore, VkFence fence, VkPipelineStageFlags* flags);
	void reset();

	// Static methods
	static VulkanCommandBuffer begin_single_time_commands(VulkanContext *context, VkCommandPool pool);
	static void end_single_time_commands(VulkanContext *context, VulkanCommandBuffer &command_buffer, VkQueue queue);

private:
	VulkanContext *context_;

	VkCommandBuffer handle_{VK_NULL_HANDLE};
	VkCommandPool pool_handle_{VK_NULL_HANDLE};

	State state_{State::NOT_ALLOCATED};

	friend VulkanRenderpass;
};

}// namespace aito