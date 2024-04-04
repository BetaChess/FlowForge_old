#pragma once
#include <vulkan/vulkan_core.h>


namespace flwfrg
{
class VulkanContext;

class VulkanFence
{
public:
	VulkanFence(VulkanContext *context, bool signaled = true);
	VulkanFence(VulkanFence &&other) noexcept;
	~VulkanFence();

	// Not copyable but movable
	VulkanFence(const VulkanFence&) = delete;
	VulkanFence& operator=(const VulkanFence&) = delete;

	[[nodiscard]] inline VkFence get_handle() const { return handle_; }
	[[nodiscard]] inline bool is_signaled() const { return signaled_; }
	bool wait(uint64_t timeout_ns);
	void reset();
	
private:
	VulkanContext *context_;

	VkFence handle_ = VK_NULL_HANDLE;
	bool signaled_ = true;
};

}
