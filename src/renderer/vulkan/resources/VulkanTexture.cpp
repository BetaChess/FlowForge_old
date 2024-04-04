#include "pch.hpp"

#include "VulkanTexture.hpp"

#include "renderer/vulkan/buffer.hpp"
#include "renderer/vulkan/vulkan_context.hpp"

namespace flwfrg
{
VulkanTexture::VulkanTexture(VulkanContext *context, uint32_t id, uint32_t width, uint32_t height, uint8_t channel_count, bool has_transparency, std::vector<Data> data)
	: context_{context},
	  id_{id},
	  width_{width},
	  height_{height},
	  channel_count_{channel_count},
	  has_transparency_{has_transparency},
	  data_{std::move(data)}
{
	VkDeviceSize image_size = width * height * channel_count;

	VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;

	// Create staging buffer
	VkBufferUsageFlagBits usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VkMemoryPropertyFlags memory_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	VulkanBuffer staging_buffer{context_, image_size, usage, memory_flags, true};

	assert(image_size == data_.size() * sizeof(Data));
	staging_buffer.load_data(data_.data(), 0, image_size, 0);

	image_ = VulkanImage(context,
						 width, height,
						 image_format,
						 VK_IMAGE_TILING_OPTIMAL,
						 VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
						 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
						 VK_IMAGE_ASPECT_COLOR_BIT,
						 true);

	VkCommandPool pool = context->vulkan_device().get_graphics_command_pool();
	VkQueue queue = context->vulkan_device().get_graphics_queue();
	VulkanCommandBuffer temp_buffer = VulkanCommandBuffer::begin_single_time_commands(context, pool);

	// Transition the layout to the optimal for recieving data
	image_.transition_layout(temp_buffer, image_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Copy data from the buffer
	image_.copy_from_buffer(temp_buffer, staging_buffer);

	// Transition to optimal read layout
	image_.transition_layout(temp_buffer, image_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VulkanCommandBuffer::end_single_time_commands(context, temp_buffer, queue);

	// Create sampler
	VkSamplerCreateInfo sampler_info{};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_TRUE;
	sampler_info.maxAnisotropy = 16;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = 0.0f;

	// Create the sampler and check the result
	if (vkCreateSampler(context->logical_device(), &sampler_info, nullptr, &sampler_) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create texture sampler");
	}

	generation_++;
}

VulkanTexture::~VulkanTexture()
{
	if (sampler_ != VK_NULL_HANDLE)
	{
		vkDestroySampler(context_->logical_device(), sampler_, nullptr);
	}
}

}// namespace flwfrg