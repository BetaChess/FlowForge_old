#pragma once

#include "imgui_impl_vulkan.h"

#include <vector>

namespace flwfrg
{
class VulkanContext;
class VulkanRenderpass;
class VulkanCommandBuffer;

class VulkanPipeline
{
public:
	~VulkanPipeline();

	// Move constructor
	VulkanPipeline(VulkanPipeline &&other) noexcept;

	// Not copyable
	VulkanPipeline(const VulkanPipeline &other) = delete;
	VulkanPipeline &operator=(const VulkanPipeline &other) = delete;

	// Create shader function
	static std::optional<VulkanPipeline> create_pipeline(VulkanContext *context,
														 const VulkanRenderpass &renderpass,
														 const std::vector<VkVertexInputAttributeDescription> &attributes,
														 const std::vector<VkDescriptorSetLayout> &descriptor_set_layouts,
														 const std::vector<VkPipelineShaderStageCreateInfo> &stages,
														 VkViewport viewport,
														 VkRect2D scissor,
														 bool is_wireframe);

	[[nodiscard]] VkPipelineLayout layout() const { return pipeline_layout_; }

	[[nodiscard]] VkPipeline handle() const { return handle_; }

	void bind(VulkanCommandBuffer &command_buffer, VkPipelineBindPoint bind_point) const;

private:
	explicit VulkanPipeline(VulkanContext *context);

	VulkanContext *context_;

	VkPipeline handle_{VK_NULL_HANDLE};
	VkPipelineLayout pipeline_layout_{VK_NULL_HANDLE};
};

}// namespace flwfrg