#include "logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace aito
{
std::shared_ptr<spdlog::logger> Logger::core_logger_s;

void Logger::init()
{
	// TODO: Make better custom pattern
	spdlog::set_pattern("%^[%T](%s:%#) %n[%l]: %v%$");
	core_logger_s = spdlog::stdout_color_mt("AITO");
	core_logger_s->set_level(spdlog::level::trace);
	
	AITO_INFO("Logger initialized successfully");
}
	
}