#pragma once

#include "../glfw_context.hpp"
#include "vulkan_context.hpp"
#include "window.hpp"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "resources/VulkanTexture.hpp"

namespace aito
{

class VulkanRenderer
{
public:
	VulkanRenderer(uint32_t initial_width, uint32_t initial_height, std::string window_name);
	~VulkanRenderer();

	bool begin_frame(float delta_time);
	void update_global_state(glm::mat4 projection, glm::mat4 view);
	bool end_frame();

	[[nodiscard]] bool should_close() const { return window_.should_close(); };

private:
	std::string window_name_;

	GLFWContext glfw_context_{};
	Window window_;
	VulkanContext vulkan_context_;
};

}// namespace aito