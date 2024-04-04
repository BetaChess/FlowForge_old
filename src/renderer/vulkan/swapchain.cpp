#include "pch.hpp"

#include "swapchain.hpp"
#include "vulkan_context.hpp"


namespace flwfrg
{
VulkanSwapchain::VulkanSwapchain(VulkanContext *context)
	: context_{context}
{
	assert(context != nullptr);
	
	FLOWFORGE_INFO("Creating initial swapchain");
	recreate_swapchain();
}

VulkanSwapchain::~VulkanSwapchain()
{
	// Destroy the views
	for (auto view: swapchain_image_views_)
	{
		vkDestroyImageView(context_->device_.logical_device_, view, nullptr);
	}
	
	vkDestroySwapchainKHR(context_->device_.logical_device_, swapchain_, nullptr);
	FLOWFORGE_INFO("Vulkan swapchain destroyed");
}

bool VulkanSwapchain::acquire_next_image(uint64_t timeout_ns, VkSemaphore image_availiable_semaphore, VkFence fence, uint32_t *out_image_index)
{
	VkResult result = vkAcquireNextImageKHR(
			context_->device_.logical_device_,
			swapchain_,
			timeout_ns,
			image_availiable_semaphore,
			fence,
			out_image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		// Trigger swapchain recreation, then boot out of the render loop.
		recreate_swapchain();
		context_->regenerate_framebuffers();
		return false;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("Failed to acquire swapchain image!");
	}

	return true;
}
bool VulkanSwapchain::present(VkQueue graphics_queue, VkQueue present_queue, VkSemaphore render_complete_semaphore, uint32_t present_image_index)
{
	VkPresentInfoKHR present_info{};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_complete_semaphore;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain_;
	present_info.pImageIndices = &present_image_index;
	present_info.pResults = nullptr;

	VkResult result = vkQueuePresentKHR(present_queue, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		recreate_swapchain();
		context_->regenerate_framebuffers();
	} else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present swapchain image!");
	}

	context_->current_frame_ = (context_->current_frame_ + 1) % max_frames_in_flight_;

	return true;
}

void VulkanSwapchain::recreate_swapchain()
{
	FLOWFORGE_TRACE("Recreating swapchain");

	vkDeviceWaitIdle(context_->device_.logical_device_);
	
	VkSwapchainKHR old_swapchain = swapchain_;
	swapchain_ = VK_NULL_HANDLE;

	// Choose swapchain surface format and throw an error if this wasn't possible
	if (!choose_swapchain_surface_format())
	{
		throw std::runtime_error("Failed to choose swapchain surface format!");
	}

	// Choose present mode
	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (size_t i = 0; i < context_->device_.swapchain_support_.present_modes.size(); i++)
	{
		if (context_->device_.swapchain_support_.present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}

	context_->device_.swapchain_support_ = context_->device_.query_swapchain_support(); // TODO: This really shouldn't be done here?

	VkExtent2D extent = context_->device_.swapchain_support_.capabilities.currentExtent;
	
	// Clamp size
	extent.width = std::clamp(
			extent.width,
			context_->device_.swapchain_support_.capabilities.minImageExtent.width,
			context_->device_.swapchain_support_.capabilities.maxImageExtent.width);
	extent.height = std::clamp(
			extent.height,
			context_->device_.swapchain_support_.capabilities.minImageExtent.height,
			context_->device_.swapchain_support_.capabilities.maxImageExtent.height);
	
	// Get image count
	uint32_t image_count = context_->device_.swapchain_support_.capabilities.minImageCount + 1;
	if (context_->device_.swapchain_support_.capabilities.maxImageCount > 0 &&
		image_count > context_->device_.swapchain_support_.capabilities.maxImageCount)
	{
		image_count = context_->device_.swapchain_support_.capabilities.maxImageCount;
	}

	// Swapchain create info
	VkSwapchainCreateInfoKHR create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = context_->surface_;
	create_info.minImageCount = image_count;
	create_info.imageFormat = swapchain_image_format_.format;
	create_info.imageColorSpace = swapchain_image_format_.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// Setup queue family indices
	uint32_t queue_family_indices[] = {
			context_->device_.graphics_queue_index_,
			context_->device_.present_queue_index_};
	if (context_->device_.graphics_queue_index_ != context_->device_.present_queue_index_)
	{
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = queue_family_indices;
	} else
	{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;    // Optional
		create_info.pQueueFamilyIndices = nullptr;// Optional
	}

	create_info.preTransform = context_->device_.swapchain_support_.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = old_swapchain;

	// Create the swapchain and check result
	if (vkCreateSwapchainKHR(
				context_->device_.logical_device_,
				&create_info,
				nullptr,
				&swapchain_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swapchain!");
	}

	if (old_swapchain != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(context_->device_.logical_device_, old_swapchain, nullptr);
	}

	// Destroy the old images (if they exist)
	for (auto view: swapchain_image_views_)
	{
		vkDestroyImageView(context_->device_.logical_device_, view, nullptr);
	}

	context_->current_frame_ = 0;
	// Get the image count and check result
	if (vkGetSwapchainImagesKHR(
				context_->device_.logical_device_,
				swapchain_,
				&image_count,
				nullptr) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to get swapchain image count");
	}
	// Resize image vector and image view vector
	swapchain_images_.resize(image_count);
	swapchain_image_views_.resize(image_count);

	// Retrieve the images
	if (vkGetSwapchainImagesKHR(
				context_->device_.logical_device_,
				swapchain_,
				&image_count,
				swapchain_images_.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to get swapchain images");
	}

	// Get the image views
	for (size_t i = 0; i < swapchain_images_.size(); i++)
	{
		VkImageViewCreateInfo image_view_create_info{};
		image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		image_view_create_info.image = swapchain_images_[i];
		image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		image_view_create_info.format = swapchain_image_format_.format;
		image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_view_create_info.subresourceRange.baseMipLevel = 0;
		image_view_create_info.subresourceRange.levelCount = 1;
		image_view_create_info.subresourceRange.baseArrayLayer = 0;
		image_view_create_info.subresourceRange.layerCount = 1;

		if (vkCreateImageView(
					context_->device_.logical_device_,
					&image_view_create_info,
					nullptr,
					&swapchain_image_views_[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create image views");
		}
	}

	// Check if depth formats are supported
	if (!context_->device_.detect_depth_format())
	{
		throw std::runtime_error("Failed to detect depth format");
	}

	// Create the depth attachment with view
	depth_attachment_ = std::make_unique<VulkanImage>(
			context_,
			extent.width,
			extent.height,
			context_->device_.depth_format_,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			true);
}

bool VulkanSwapchain::choose_swapchain_surface_format()
{
	for (auto format: context_->device_.swapchain_support_.formats)
	{
		if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
			format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			swapchain_image_format_ = format;
			return true;
		}
	}
	return false;
}


}// namespace flwfrg