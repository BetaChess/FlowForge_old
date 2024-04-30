#include "pch.hpp"

#include "image.hpp"

#include "vulkan_context.hpp"
#include "command_buffer.hpp"
#include "buffer.hpp"

namespace flwfrg
{


VulkanImage::VulkanImage(
		VulkanContext *context,
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags memory_flags,
		VkImageAspectFlags aspect_flags,
		bool create_view)
	: context_{context},
	  width_{width},
	  height_{height}
{
	assert(context != nullptr);
	
	// Create info struct
	VkImageCreateInfo image_info{};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.format = format;
	image_info.tiling = tiling;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usage;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	// Create the image and check result
	if (vkCreateImage(
				context_->device_.logical_device_,
				&image_info,
				nullptr,
				&image_handle_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image!");
	}

	// Query memory requirements
	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(
			context_->device_.logical_device_,
			image_handle_,
			&memory_requirements);

	int32_t memory_type = context->find_memory_index(memory_requirements.memoryTypeBits, memory_flags);
	if (memory_type == -1)
	{
		throw std::runtime_error("Failed to find suitable memory type");
	}

	// Allocate memory
	VkMemoryAllocateInfo memory_allocate_info{};
	memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_allocate_info.allocationSize = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = memory_type;

	// Do the allocation and check result
	if (vkAllocateMemory(
				context_->device_.logical_device_,
				&memory_allocate_info,
				nullptr,
				&memory_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate image memory");
	}

	// Bind the memory
	if (vkBindImageMemory(
				context_->device_.logical_device_,
				image_handle_,
				memory_,
				0) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to bind image memory");
	}

	if (create_view)
	{
		view_create(format, aspect_flags);
	}
}

VulkanImage::~VulkanImage()
{
	if (view_)
	{
		vkDestroyImageView(context_->device_.logical_device_, view_, nullptr);
	}
	if (image_handle_)
	{
		vkDestroyImage(context_->device_.logical_device_, image_handle_, nullptr);
	}
	if (memory_)
	{
		vkFreeMemory(context_->device_.logical_device_, memory_, nullptr);
	}
}
VulkanImage::VulkanImage(VulkanImage &&other) noexcept
	: context_{other.context_},
	  image_handle_{other.image_handle_},
	  memory_{other.memory_},
	  view_{other.view_},
	  width_{other.width_},
	  height_{other.height_}
{
	other.image_handle_ = VK_NULL_HANDLE;
	other.memory_ = VK_NULL_HANDLE;
	other.view_ = VK_NULL_HANDLE;
	other.width_ = 0;
	other.height_ = 0;
}
VulkanImage &VulkanImage::operator=(VulkanImage &&other) noexcept
{
	if (this != &other)
	{
		if (view_)
		{
			vkDestroyImageView(context_->device_.logical_device_, view_, nullptr);
		}
		if (image_handle_)
		{
			vkDestroyImage(context_->device_.logical_device_, image_handle_, nullptr);
		}
		if (memory_)
		{
			vkFreeMemory(context_->device_.logical_device_, memory_, nullptr);
		}

		context_ = other.context_;
		image_handle_ = other.image_handle_;
		memory_ = other.memory_;
		view_ = other.view_;
		width_ = other.width_;
		height_ = other.height_;

		other.image_handle_ = VK_NULL_HANDLE;
		other.memory_ = VK_NULL_HANDLE;
		other.view_ = VK_NULL_HANDLE;
		other.width_ = 0;
		other.height_ = 0;
	}

	return *this;
}

void VulkanImage::transition_layout(VulkanCommandBuffer &command_buffer, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = context_->device_.graphics_queue_index_;
	barrier.dstQueueFamilyIndex = context_->device_.graphics_queue_index_;
	barrier.image = image_handle_;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags destination_stage;

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
			command_buffer.get_handle(),
			source_stage,
			destination_stage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
}

void VulkanImage::copy_from_buffer(VulkanCommandBuffer &command_buffer, VulkanBuffer &buffer)
{
	// Region to copy
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width_, height_, 1};

	vkCmdCopyBufferToImage(
			command_buffer.get_handle(),
			buffer.get_handle(),
			image_handle_,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region);
}

void VulkanImage::view_create(VkFormat format, VkImageAspectFlags aspect_flags)
{
	VkImageViewCreateInfo view_info{};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = image_handle_;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.format = format;
	view_info.subresourceRange.aspectMask = aspect_flags;
	
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;

	if (vkCreateImageView(
				context_->device_.logical_device_,
				&view_info,
				nullptr,
				&view_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create image view");
	}
}

}// namespace flwfrg