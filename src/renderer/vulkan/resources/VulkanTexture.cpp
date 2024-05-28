#include "pch.hpp"

#include "VulkanTexture.hpp"

#include "renderer/vulkan/buffer.hpp"
#include "renderer/vulkan/vulkan_context.hpp"

#include <stb_image.h>

namespace flwfrg
{
VulkanTexture::VulkanTexture(VulkanContext *context, uint32_t id, uint32_t width, uint32_t height, bool has_transparency, std::vector<Data> data)
	: context_{context},
	  id_{id},
	  width_{width},
	  height_{height},
	  has_transparency_{has_transparency},
	  data_{std::move(data)}
{
	VkDeviceSize image_size = width * height * sizeof(Data);

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
VulkanTexture::VulkanTexture(VulkanTexture &&other) noexcept
	: context_(other.context_),
	  id_(other.id_),
	  width_(other.width_),
	  height_(other.height_),
	  has_transparency_(other.has_transparency_),
	  generation_(other.generation_),
	  data_(std::move(other.data_)),
	  image_(std::move(other.image_)),
	  sampler_(other.sampler_)
{
	other.sampler_ = VK_NULL_HANDLE;
}
VulkanTexture &VulkanTexture::operator=(VulkanTexture &&other) noexcept
{
	if (this != &other)
	{
		if (sampler_ != VK_NULL_HANDLE)
		{
			vkDestroySampler(context_->logical_device(), sampler_, nullptr);
		}

		context_ = other.context_;
		id_ = other.id_;
		width_ = other.width_;
		height_ = other.height_;
		has_transparency_ = other.has_transparency_;
		generation_ = other.generation_;
		data_ = std::move(other.data_);
		image_ = std::move(other.image_);
		sampler_ = other.sampler_;

		other.sampler_ = VK_NULL_HANDLE;
	}

	return *this;
}

bool VulkanTexture::load_texture_from_file(std::string texture_name)
{
	std::string path = "assets/textures/" + texture_name + ".png";
	const int32_t required_channel_count = 4;
	stbi_set_flip_vertically_on_load(true);
	
	int32_t width, height, channel_count;

	uint8_t *data = stbi_load(path.c_str(),
							  &width,
							  &height,
							  &channel_count,
							  required_channel_count);
	

	if (stbi_failure_reason())
	{
		FLOWFORGE_WARN("Failed to load texture '{}', {}", path, stbi_failure_reason());
	}
	
	if (data == nullptr)
	{
		return false;
	}

	uint32_t generation = generation_;
	generation_ = std::numeric_limits<uint32_t>::max();
	uint64_t total_size = width * height * required_channel_count;

	bool has_transparency = false;
	for (size_t i = 0; i < total_size; i += 4)
	{
		uint8_t a = data[i + 3];
		if (a < 255)
		{
			has_transparency = true;
			break;
		}
	}

	std::vector<Data> data_vec;
	data_vec.resize(total_size / sizeof(Data));
	for (size_t i = 0; i < total_size; i += 4)
	{
		data_vec[i / 4].color.r = data[i] / 255.0f;
		data_vec[i / 4].color.g = data[i + 1] / 255.0f;
		data_vec[i / 4].color.b = data[i + 2] / 255.0f;
		data_vec[i / 4].color.a = data[i + 3] / 255.0f;
	}

	stbi_image_free(data);

	*this = VulkanTexture{context_, id_, static_cast<uint32_t>(width), static_cast<uint32_t>(height), has_transparency, data_vec};

	if (generation == std::numeric_limits<uint32_t>::max())
		generation_ = 0;
	else
		generation_ = generation + 1;
	
	return true;
}

}// namespace flwfrg