#include "pch.hpp"

#include "buffer.hpp"

#include "vulkan_context.hpp"

namespace aito
{


VulkanBuffer::VulkanBuffer(VulkanContext *context, uint64_t size, VkBufferUsageFlagBits usage, uint32_t memory_property_flags, bool bind_on_create)
	: context_(context),
	  total_size_(size),
	  usage_(usage),
	  locked_(false),
	  memory_property_flags_(memory_property_flags)
{
	assert(context != nullptr);
	
	VkBufferCreateInfo buffer_create_info{};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = size;
	buffer_create_info.usage = usage;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Only used in a single queue

	if (vkCreateBuffer(context->logical_device(), &buffer_create_info, nullptr, &handle_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create buffer");
	}

	// Gather memory requirements
	VkMemoryRequirements memory_requirements;
	vkGetBufferMemoryRequirements(context->logical_device(), handle_, &memory_requirements);
	memory_index_ = context->find_memory_index(memory_requirements.memoryTypeBits, memory_property_flags_);
	if (memory_index_ == -1)
	{
		throw std::runtime_error("Failed to find suitable memory type");
	}

	// Allocate memory info
	VkMemoryAllocateInfo memory_allocate_info{};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = static_cast<uint32_t>(memory_index_);

	// Allocate the memory
	if (vkAllocateMemory(context->logical_device(), &memory_allocate_info, nullptr, &memory_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate memory");
	}

	if (bind_on_create)
	{
		bind(0);
	}
}

VulkanBuffer::~VulkanBuffer()
{
	if (handle_ != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(context_->logical_device(), handle_, nullptr);
	}
	if (memory_ != VK_NULL_HANDLE)
	{
		vkFreeMemory(context_->logical_device(), memory_, nullptr);
	}
}
VulkanBuffer::VulkanBuffer(VulkanBuffer &&other) noexcept
	: context_(other.context_),
	  total_size_(other.total_size_),
	  handle_(other.handle_),
	  usage_(other.usage_),
	  locked_(other.locked_),
	  memory_(other.memory_),
	  memory_index_(other.memory_index_),
	  memory_property_flags_(other.memory_property_flags_)
{
	other.handle_ = VK_NULL_HANDLE;
	other.memory_ = VK_NULL_HANDLE;
}
VulkanBuffer &VulkanBuffer::operator=(VulkanBuffer &&other) noexcept
{
	if (this != &other)
	{
		if (handle_ != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(context_->logical_device(), handle_, nullptr);
		}
		if (memory_ != VK_NULL_HANDLE)
		{
			vkFreeMemory(context_->logical_device(), memory_, nullptr);
		}

		context_ = other.context_;
		total_size_ = other.total_size_;
		handle_ = other.handle_;
		usage_ = other.usage_;
		locked_ = other.locked_;
		memory_ = other.memory_;
		memory_index_ = other.memory_index_;
		memory_property_flags_ = other.memory_property_flags_;

		other.handle_ = VK_NULL_HANDLE;
		other.memory_ = VK_NULL_HANDLE;
	}
	return *this;
}

void VulkanBuffer::resize(uint64_t new_size, VkQueue queue, VkCommandPool pool)
{
	// Create new buffer
	VulkanBuffer new_buffer{context_, new_size, usage_, memory_property_flags_, true};

	// Copy the data
	copy_to(new_buffer, 0, new_size, 0, pool, VK_NULL_HANDLE, queue);

	// Wait for device to be idle
	vkDeviceWaitIdle(context_->logical_device());

	if (handle_ != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(context_->logical_device(), handle_, nullptr);
	}
	if (memory_ != VK_NULL_HANDLE)
	{
		vkFreeMemory(context_->logical_device(), memory_, nullptr);
	}

	// Move the new buffer to this
	*this = std::move(new_buffer);
}
void *VulkanBuffer::lock_memory(uint64_t offset, uint64_t size, uint32_t flags)
{
	void *data;
	if (vkMapMemory(context_->logical_device(), memory_, offset, size, flags, &data) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to map memory");
	}
	locked_ = true;
	return data;
}
void VulkanBuffer::unlock_memory()
{
	vkUnmapMemory(context_->logical_device(), memory_);
	locked_ = false;
}

void VulkanBuffer::bind(uint64_t offset)
{
	vkBindBufferMemory(context_->logical_device(), handle_, memory_, offset);
}
void VulkanBuffer::load_data(const void *data, uint64_t offset, uint64_t size, uint32_t flags)
{
	void *mapped_memory = lock_memory(offset, size, flags);
	memcpy(mapped_memory, data, size);
	unlock_memory();
}
void VulkanBuffer::copy_to(VulkanBuffer &dst, uint64_t dst_offset, uint64_t dst_size, uint64_t src_offset, VkCommandPool pool, VkFence fence, VkQueue queue)
{
	vkQueueWaitIdle(queue);
	
	// Create a one time use command buffer
	VulkanCommandBuffer command_buffer = VulkanCommandBuffer::begin_single_time_commands(context_, pool);

	// Copy the buffer
	VkBufferCopy copy_region{};
	copy_region.srcOffset = src_offset;
	copy_region.dstOffset = dst_offset;
	copy_region.size = dst_size;

	vkCmdCopyBuffer(command_buffer.get_handle(), handle_, dst.handle_, 1, &copy_region);

	// End the command buffer
	VulkanCommandBuffer::end_single_time_commands(context_, command_buffer, queue);
}


}// namespace aito