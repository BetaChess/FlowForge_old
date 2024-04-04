#pragma once

#include "renderer/vulkan/renderer.hpp"

namespace aito
{

class Application
{
public:
	Application();
	~Application();

	void run();

private:
	VulkanRenderer renderer_{1280, 800, "TestName"};
};

}// namespace aito