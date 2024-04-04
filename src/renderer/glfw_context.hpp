#pragma once

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace aito
{

class GLFWContext
{
public:
	GLFWContext();
	~GLFWContext();
	
private:
};

}
