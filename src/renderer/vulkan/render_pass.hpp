#pragma once

#include "command_buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <vulkan/vulkan_core.h>

namespace flwfrg
{
class VulkanFrameBuffer;
class VulkanContext;

class VulkanRenderpass
{
public:
	enum class State
	{
		READY,
		RECORDING,
		IN_RENDER_PASS,
		RECORDING_ENDED,
		SUBMITTED,
		NOT_ALLOCATED
	};
	
public:
	VulkanRenderpass(VulkanContext *context, glm::vec4 draw_area, glm::vec4 clear_color, float depth, uint32_t stencil);
	~VulkanRenderpass();

	// Not copyable or movable
	VulkanRenderpass(const VulkanRenderpass&) = delete;
	VulkanRenderpass& operator=(const VulkanRenderpass&) = delete;
	VulkanRenderpass(VulkanRenderpass&&) = delete;
	VulkanRenderpass& operator=(VulkanRenderpass&&) = delete;

	[[nodiscard]] inline VkRenderPass get_handle() const { return handle_;};

	void set_render_area(glm::vec4 draw_area);

	void begin(VulkanCommandBuffer& command_buffer, VkFramebuffer frame_buffer);
	void end(VulkanCommandBuffer& command_buffer);

private:
	VulkanContext * context_;
	
	VkRenderPass handle_;
	glm::vec4 draw_area_;
	glm::vec4 clear_color_;

	float depth;
	uint32_t stencil;

	State state_;

	friend VulkanFrameBuffer;
};

}