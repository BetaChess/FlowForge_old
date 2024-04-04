#pragma once

#include "../glfw_context.hpp"

#include <string>
#include <utility>

namespace aito
{

class Window
{
public:
	// Constructor(s)
	Window(size_t w, size_t h, const std::string &name);

	// Delete copy constructor and assignment operator.
	Window(const Window &) = delete;
	Window &operator=(const Window &) = delete;

	// Destructor
	~Window();

	///// Public methods

	[[nodiscard]] inline bool should_close() const { return glfwWindowShouldClose(window_); }
	[[nodiscard]] inline uint32_t get_width() const { return frame_buffer_width_; };
	[[nodiscard]] inline uint32_t get_height() const { return frame_buffer_height_; };
	[[nodiscard]] inline VkExtent2D get_extent() const { return {frame_buffer_width_, frame_buffer_height_}; };
	[[nodiscard]] inline bool was_window_resized() const { return frame_buffer_resized_; };
	inline void reset_window_resized_flag() { frame_buffer_resized_ = false; };
	[[nodiscard]] inline GLFWwindow *get_glfw_window_ptr() const
	{
		return window_;
	}
	[[nodiscard]] VkSurfaceKHR create_window_surface(VkInstance instance) const;
	inline void register_resize_callback(std::function<void(void *)> callback, void *context)
	{
		resize_callback_ = std::move(callback);
		callback_param_ptr_ = context;
	}


private:
	// Private members
	GLFWwindow *window_;
	std::string windowName_;

	uint32_t frame_buffer_width_, frame_buffer_height_;
	bool frame_buffer_resized_ = false;

	std::function<void(void *)> resize_callback_ = nullptr;
	void *callback_param_ptr_ = nullptr;

	// Frame buffer resize method
	static void framebuffer_resize_callback(GLFWwindow *window, int width, int height);
};

}// namespace aito
