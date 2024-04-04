#include "pch.hpp"

#include "vulkan_context.hpp"
#include "frame_buffer.hpp"
#include "render_pass.hpp"

#include <utility>

namespace aito
{

VulkanFrameBuffer::VulkanFrameBuffer(VulkanContext *context, VulkanRenderpass &renderpass, uint32_t width, uint32_t height, std::vector<VkImageView> attachments)
	: context_{context},
	  renderpass_{renderpass},
	  attachments_{std::move(attachments)}
{
	assert(context != nullptr);
	
	VkFramebufferCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	create_info.renderPass = renderpass.handle_;
	create_info.attachmentCount = static_cast<uint32_t>(attachments_.size());
	create_info.pAttachments = attachments_.data();
	create_info.width = width;
	create_info.height = height;
	create_info.layers = 1;

	if (vkCreateFramebuffer(context_->device_.logical_device_, &create_info, nullptr, &handle_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create framebuffer");
	}
	AITO_TRACE("Framebuffer created");
}

VulkanFrameBuffer::VulkanFrameBuffer(VulkanFrameBuffer &&other) noexcept
	: context_{other.context_},
	  handle_{other.handle_},
	  renderpass_{other.renderpass_},
	  attachments_{std::move(other.attachments_)}
{
	other.handle_ = VK_NULL_HANDLE;
}

VulkanFrameBuffer::~VulkanFrameBuffer()
{
	if (handle_ != VK_NULL_HANDLE)
	{
		vkDestroyFramebuffer(context_->device_.logical_device_, handle_, nullptr);
		AITO_TRACE("Framebuffer destroyed");
	}
}

}// namespace aito