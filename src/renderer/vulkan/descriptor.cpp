#include "pch.hpp"

#include "descriptor.hpp"

#include "vulkan_context.hpp"

namespace flwfrg
{


VulkanDescriptorPool::VulkanDescriptorPool(VulkanContext *context, VkDescriptorPoolCreateInfo create_info)
	: context_{context}, pool_{VK_NULL_HANDLE}
{
	assert(context_ != nullptr);

	if (vkCreateDescriptorPool(context_->logical_device(), &create_info, nullptr, &pool_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}
}
VulkanDescriptorPool::~VulkanDescriptorPool()
{
	if (pool_ != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(context_->logical_device(), pool_, nullptr);
		pool_ = VK_NULL_HANDLE;
	}
}
VulkanDescriptorPool::VulkanDescriptorPool(VulkanDescriptorPool &&other) noexcept
	: context_{other.context_}, pool_{other.pool_}
{
	other.pool_ = VK_NULL_HANDLE;
}

VulkanDescriptorPool &VulkanDescriptorPool::operator=(VulkanDescriptorPool &&other) noexcept
{
	if (this != &other)
	{
		if (pool_ != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(context_->logical_device(), pool_, nullptr);
		}

		context_ = other.context_;
		pool_ = other.pool_;

		other.pool_ = VK_NULL_HANDLE;
	}

	return *this;
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanContext *context, VkDescriptorSetLayoutCreateInfo create_info)
	: context_{context}, layout_{VK_NULL_HANDLE}
{
	assert(context_ != nullptr);

	if (vkCreateDescriptorSetLayout(context_->logical_device(), &create_info, nullptr, &layout_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor set layout");
	}
}
VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
	if (layout_ != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorSetLayout(context_->logical_device(), layout_, nullptr);
		layout_ = VK_NULL_HANDLE;
	}
}
VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VulkanDescriptorSetLayout &&other) noexcept
	: context_{other.context_}, layout_{other.layout_}
{
	other.layout_ = VK_NULL_HANDLE;
}
VulkanDescriptorSetLayout &VulkanDescriptorSetLayout::operator=(VulkanDescriptorSetLayout &&other) noexcept
{
	if (this != &other)
	{
		if (layout_ != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(context_->logical_device(), layout_, nullptr);
		}

		context_ = other.context_;
		layout_ = other.layout_;

		other.layout_ = VK_NULL_HANDLE;
	}

	return *this;
}

}// namespace flwfrg