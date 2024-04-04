#include "pch.hpp"

#include "glfw_context.hpp"

static void glfw_error_callback(int error, const char* description)
{
	AITO_ERROR("GLFW Error {}: {}", error, description);
}

namespace aito
{

GLFWContext::GLFWContext()
{
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
	{
		AITO_FATAL("GLFW Failed to initialize");
		throw std::runtime_error("GLFW Failed to initialize");
	}
	AITO_INFO("GLFW Initialised successfully");
}

GLFWContext::~GLFWContext()
{
	glfwTerminate();
	AITO_INFO("GLFW terminated successfully");
}

}