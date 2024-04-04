#include "pch.hpp"

#include "../vulkan_context.hpp"
#include "shader_stage.hpp"

#include <fstream>
#include <optional>
#include <string>

namespace flwfrg
{

VulkanShaderStage::~VulkanShaderStage()
{
	if (handle_ != VK_NULL_HANDLE)
	{
		vkDestroyShaderModule(context_->logical_device(), handle_, nullptr);
	}
}
VulkanShaderStage::VulkanShaderStage(VulkanShaderStage &&other) noexcept
	: context_(other.context_),
	  create_info(other.create_info),
	  handle_(other.handle_),
	  shader_stage_create_info(other.shader_stage_create_info)
{
	other.handle_ = VK_NULL_HANDLE;
}

std::optional<VulkanShaderStage> VulkanShaderStage::create_shader_module(VulkanContext *context, const std::string &name, VkShaderStageFlagBits shader_stage_flag)
{
	assert(context != nullptr);
	
	std::string file_name{};
	try
	{
		file_name = "assets/shaders/" + name + get_shader_stage_file_extension(shader_stage_flag);
	} catch (const std::runtime_error &e)
	{
		FLOWFORGE_ERROR(e.what());
		FLOWFORGE_ERROR("Shader stage flag was {}", shader_stage_flag);
		return std::nullopt;
	}
	VulkanShaderStage return_stage{context};
	// Read in the file
	std::ifstream file(file_name, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		FLOWFORGE_ERROR("Failed to open file: {}", file_name);
		return std::nullopt;
	}
	std::vector<uint8_t> file_buffer;
	// Read the entire file as binary
	file.seekg(0, std::ios::end);
	file_buffer.resize(file.tellg());
	file.seekg(0);
	file.read(reinterpret_cast<char *>(file_buffer.data()),
			  static_cast<long>(file_buffer.size()));

	// Set shader stage info
	return_stage.create_info.codeSize = file_buffer.size();
	return_stage.create_info.pCode = reinterpret_cast<const uint32_t *>(file_buffer.data());

	// Close the file
	file.close();

	// Create the shader module and check the result
	if (vkCreateShaderModule(context->logical_device(), &return_stage.create_info, nullptr, &return_stage.handle_) != VK_SUCCESS)
	{
		FLOWFORGE_ERROR("Failed to create shader module");
		return std::nullopt;
	}

	return_stage.create_info.pCode = nullptr;

	// Set shader stage create info
	return_stage.shader_stage_create_info.stage = shader_stage_flag;
	return_stage.shader_stage_create_info.module = return_stage.handle_;
	return_stage.shader_stage_create_info.pName = "main";// Shader entry point

	return return_stage;
}

VulkanShaderStage::VulkanShaderStage(VulkanContext *context)
	: context_(context)
{
}

}// namespace flwfrg
