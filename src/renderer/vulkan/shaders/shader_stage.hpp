#pragma once

#include "imgui_impl_vulkan.h"

namespace aito
{
class VulkanContext;

constexpr const char *get_shader_stage_file_extension(VkShaderStageFlagBits stage)
{
	switch (stage)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:
			return ".vert.spv";
		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return ".frag.spv";
		case VK_SHADER_STAGE_COMPUTE_BIT:
			return ".comp.spv";
		case VK_SHADER_STAGE_GEOMETRY_BIT:
			return ".geom.spv";
		case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
			return ".tesc.spv";
		case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
			return ".tese.spv";
		default:
			throw std::runtime_error("Unknown or invalid shader stage");
	}
}

class VulkanShaderStage
{
public:
	~VulkanShaderStage();

	// Move constructor
	VulkanShaderStage(VulkanShaderStage &&other) noexcept;

	// Not copyable
	VulkanShaderStage(const VulkanShaderStage &other) = delete;
	VulkanShaderStage &operator=(const VulkanShaderStage &other) = delete;
	
	static std::optional<VulkanShaderStage> create_shader_module(VulkanContext *context,
												  const std::string& name,
												  VkShaderStageFlagBits shader_stage_flag);

	inline VkPipelineShaderStageCreateInfo get_shader_stage_create_info() const { return shader_stage_create_info; }

private:
	explicit VulkanShaderStage(VulkanContext * context);

	VulkanContext * context_;

	VkShaderModuleCreateInfo create_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	VkShaderModule handle_ = VK_NULL_HANDLE;
	VkPipelineShaderStageCreateInfo shader_stage_create_info{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
};

}// namespace aito
