#pragma once

#include <vulkan/vulkan_core.h>

namespace aito
{
class VulkanContext;

class VulkanBuffer
{
public:
	VulkanBuffer() = default;
	VulkanBuffer(VulkanContext *context,
				 uint64_t size,
				 VkBufferUsageFlagBits usage,
				 uint32_t memory_property_flags,
				 bool bind_on_create);
	~VulkanBuffer();

	// Not copyable but movable
	VulkanBuffer(const VulkanBuffer &) = delete;
	VulkanBuffer &operator=(const VulkanBuffer &) = delete;
	VulkanBuffer(VulkanBuffer &&other) noexcept;
	VulkanBuffer &operator=(VulkanBuffer &&other) noexcept;

	// Methods

	[[nodiscard]] inline VkBuffer get_handle() const { return handle_; };
	
	void resize(uint64_t new_size, VkQueue queue, VkCommandPool pool);
	
	void *lock_memory(uint64_t offset, uint64_t size, uint32_t flags);
	void unlock_memory();

	void bind(uint64_t offset);

	void load_data(const void *data, uint64_t offset, uint64_t size, uint32_t flags);

	void copy_to(VulkanBuffer &dst,
				 uint64_t dst_offset,
				 uint64_t dst_size,
				 uint64_t src_offset,
				 VkCommandPool pool,
				 VkFence fence,
				 VkQueue queue);

private:
	VulkanContext *context_ = nullptr;

	uint64_t total_size_ = 0;
	VkBuffer handle_ = VK_NULL_HANDLE;
	VkBufferUsageFlagBits usage_ = static_cast<VkBufferUsageFlagBits>(0);
	bool locked_ = false;
	VkDeviceMemory memory_ = VK_NULL_HANDLE;
	int32_t memory_index_ = -1;
	uint32_t memory_property_flags_ = 0;
};

}// namespace aito
