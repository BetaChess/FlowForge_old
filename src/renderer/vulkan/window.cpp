#include "pch.hpp"

#include "window.hpp"

#include <GL/gl.h>


namespace flwfrg
{
Window::Window(size_t w, size_t h, const std::string& name)
	: frame_buffer_width_(static_cast<uint32_t>(w)), frame_buffer_height_(static_cast<uint32_t>(h)), windowName_(name)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	window_ = glfwCreateWindow(static_cast<int>(w), static_cast<int>(h), name.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(window_, this);
	glfwSetFramebufferSizeCallback(window_, framebuffer_resize_callback);
	
	if (!glfwVulkanSupported())
	{
		FLOWFORGE_FATAL("Vulkan isn't supported");
	}
	
	FLOWFORGE_INFO("GLFW window created");
}

Window::~Window()
{
	glfwDestroyWindow(window_);
	FLOWFORGE_INFO("GLFW window destroyed");
}
VkSurfaceKHR Window::create_window_surface(VkInstance instance) const
{
	VkSurfaceKHR surface;
	
	if (glfwCreateWindowSurface(instance, window_, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create window surface! ");
	}
	
	return surface;
}

void Window::framebuffer_resize_callback(GLFWwindow *window, int width, int height)
{
	auto vulkanWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	
	vulkanWindow->frame_buffer_resized_ = true;
	vulkanWindow->frame_buffer_width_ = width;
	vulkanWindow->frame_buffer_height_ = height;

	if (vulkanWindow->resize_callback_)
		vulkanWindow->resize_callback_(vulkanWindow->callback_param_ptr_);
}

} // flwfrg