#include "pch.hpp"

#include "application.hpp"

namespace aito
{

Application::Application()
{
	
}


Application::~Application()
{
}

void Application::run()
{
	while (!renderer_.should_close())
	{
		if (!renderer_.begin_frame(0.01f))
			continue;

		renderer_.update_global_state(glm::mat4(1.0f), glm::mat4(1.0f));

		
		renderer_.end_frame();
	}
}

}// namespace aito
