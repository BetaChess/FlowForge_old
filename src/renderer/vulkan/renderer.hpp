#pragma once

#include "../glfw_context.hpp"
#include "vulkan_context.hpp"
#include "window.hpp"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "resources/VulkanTexture.hpp"

namespace flwfrg
{

class VulkanRenderer
{
public:
	struct RendererState
	{
		glm::mat4 projection;
		glm::mat4 view;
		float near_clip;
		float far_clip;

		VulkanTexture default_texture;
	};
	
public:
	VulkanRenderer(uint32_t initial_width, uint32_t initial_height, std::string window_name);
	~VulkanRenderer();

	bool begin_frame(float delta_time);
	bool end_frame();
	void update_global_state(glm::mat4 projection, glm::mat4 view);
	void update_projection(glm::mat4 projection);
	void update_view(glm::mat4 view);
	void update_near_clip(float near_clip);
	void update_far_clip(float far_clip);

	[[nodiscard]] bool should_close() const { return window_.should_close(); };

private:
	std::string window_name_;

	GLFWContext glfw_context_{};
	Window window_;
	VulkanContext vulkan_context_;

	RendererState state_;

	void generate_default_texture();
};

}// namespace flwfrg