#include "pch.hpp"

#include "command_buffer.hpp"

#include "vulkan_context.hpp"

namespace flwfrg
{


VulkanCommandBuffer::VulkanCommandBuffer(VulkanContext *context, VkCommandPool pool, bool is_primary)
	: context_(context)
{
	assert(context != nullptr);
	
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandPool = pool;
	alloc_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	alloc_info.commandBufferCount = 1;
	alloc_info.pNext = nullptr;

	if (vkAllocateCommandBuffers(context_->device_.logical_device_, &alloc_info, &handle_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate command buffer");
	}

	pool_handle_ = pool;
	state_ = State::READY;
	FLOWFORGE_TRACE("Command buffer created successfully");
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanCommandBuffer &&other) noexcept
	: context_{other.context_},
	  handle_{other.handle_},
	  pool_handle_{other.pool_handle_},
	  state_{other.state_}
{
	other.handle_ = VK_NULL_HANDLE;
	other.pool_handle_ = VK_NULL_HANDLE;
	other.state_ = State::NOT_ALLOCATED;
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
	if (state_ != State::NOT_ALLOCATED)
	{
		vkFreeCommandBuffers(context_->device_.logical_device_, pool_handle_, 1, &handle_);
		FLOWFORGE_TRACE("Command buffer destroyed");
	}
}

void VulkanCommandBuffer::begin(bool is_single_use, bool is_renderpass_continue, bool is_simultaneous_use)
{
	if (state_ != State::READY)
	{
		throw std::runtime_error("Command buffer not ready to begin");
	}

	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.flags |= is_single_use ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
	begin_info.flags |= is_renderpass_continue ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 0;
	begin_info.flags |= is_simultaneous_use ? VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT : 0;

	if (vkBeginCommandBuffer(handle_, &begin_info) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to begin recording command buffer");
	}

	state_ = State::RECORDING;
}

void VulkanCommandBuffer::end()
{
	if (state_ != State::RECORDING)
	{
		throw std::runtime_error("Command buffer not in progress");
	}

	if (vkEndCommandBuffer(handle_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to end recording command buffer");
	}

	state_ = State::RECORDING_ENDED;
}

void VulkanCommandBuffer::submit(VkQueue queue, VkSemaphore wait_semaphore, VkSemaphore signal_semaphore, VkFence fence, VkPipelineStageFlags* flags = nullptr)
{
	if (state_ != State::RECORDING_ENDED)
	{
		throw std::runtime_error("Command buffer not ready to submit");
	}

	VkSubmitInfo submit_info{};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = wait_semaphore != VK_NULL_HANDLE ? 1 : 0;
	submit_info.pWaitSemaphores = &wait_semaphore;
	submit_info.pWaitDstStageMask = flags;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &handle_;
	submit_info.signalSemaphoreCount = signal_semaphore != VK_NULL_HANDLE ? 1 : 0;
	submit_info.pSignalSemaphores = &signal_semaphore;

	if (vkQueueSubmit(queue, 1, &submit_info, fence) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit command buffer");
	}

	state_ = State::SUBMITTED;
}

void VulkanCommandBuffer::reset()
{
	if (state_ == State::RECORDING)
	{
		FLOWFORGE_WARN("Command buffer reset while recording");
	}

	if (vkResetCommandBuffer(handle_, 0) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to reset command buffer");
	}

	state_ = State::READY;
}
VulkanCommandBuffer VulkanCommandBuffer::begin_single_time_commands(VulkanContext *context, VkCommandPool pool)
{
	VulkanCommandBuffer command_buffer{context, pool, true};
	command_buffer.begin(true, false, false);
	return command_buffer;
}
void VulkanCommandBuffer::end_single_time_commands(VulkanContext *context, VulkanCommandBuffer &command_buffer, VkQueue queue)
{
	command_buffer.end();
	command_buffer.submit(queue, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE);
	// Wait for it to finish and check result
	if (vkQueueWaitIdle(queue) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to wait for queue to finish");
	}
	command_buffer.reset();
}

}// namespace flwfrg