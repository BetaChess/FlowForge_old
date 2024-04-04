#include "pch.hpp"

#include "glfw_context.hpp"

static void glfw_error_callback(int error, const char* description)
{
	FLOWFORGE_ERROR("GLFW Error {}: {}", error, description);
}

namespace flwfrg
{

GLFWContext::GLFWContext()
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
	{
		FLOWFORGE_FATAL("GLFW Failed to initialize");
		throw std::runtime_error("GLFW Failed to initialize");
	}
	FLOWFORGE_INFO("GLFW Initialised successfully");
}

GLFWContext::~GLFWContext()
{
	glfwTerminate();
	FLOWFORGE_INFO("GLFW terminated successfully");
}

}