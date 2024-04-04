#include "pch.hpp"

#include "vulkan_fence.hpp"

#include "vulkan_context.hpp"

namespace aito
{

VulkanFence::VulkanFence(VulkanContext *context, bool signaled)
	: context_(context), signaled_(signaled)
{
	assert(context != nullptr);
	
	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = signaled_ ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	if (vkCreateFence(context_->device_.logical_device_, &fence_info, nullptr, &handle_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create fence");
	}
	AITO_TRACE("Fence created");
}

VulkanFence::VulkanFence(VulkanFence &&other) noexcept
	: context_{other.context_},
	  handle_{other.handle_},
	  signaled_{other.signaled_}
{
	other.handle_ = VK_NULL_HANDLE;
	other.signaled_ = false;
}


VulkanFence::~VulkanFence()
{
	if (handle_ != VK_NULL_HANDLE)
	{
		vkDestroyFence(context_->device_.logical_device_, handle_, nullptr);
		AITO_TRACE("Fence destroyed");
	}
}

bool VulkanFence::wait(uint64_t timeout_ns)
{
	if (signaled_)
	{
		return true;
	}

	// Wait and check result
	VkResult result = vkWaitForFences(context_->device_.logical_device_, 1, &handle_, VK_TRUE, timeout_ns);

	switch (result)
	{
	case VK_SUCCESS:
		signaled_ = true;
		return true;
	case VK_TIMEOUT:
		AITO_WARN("Fence wait timed out");
		return false;
	case VK_ERROR_DEVICE_LOST:
		AITO_ERROR("Device lost while waiting for fence");
		return false;
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		AITO_ERROR("Out of host memory while waiting for fence");
		return false;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		AITO_ERROR("Out of device memory while waiting for fence");
		return false;
	default:
		AITO_ERROR("Failed to wait for fence. Unkown Error occurred. ");
		return false;
	}
}

void VulkanFence::reset()
{
	if (signaled_)
	{
		// Reset fence and check result
		if (vkResetFences(context_->device_.logical_device_, 1, &handle_) != VK_SUCCESS)
		{
			AITO_ERROR("Failed to reset fence");
			return;
		}
		
		signaled_ = false;
	}
}

}// namespace aito