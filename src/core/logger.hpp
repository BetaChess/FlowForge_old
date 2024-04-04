#pragma once
// Set the logger to log all, so aito can decide what to log itself.
#define SPDLOG_ACTIVE_LEVEL 0


#include <memory>

#include <spdlog/spdlog.h>
#undef near
#undef far



namespace aito
{

class Logger
{
private:
	static std::shared_ptr<spdlog::logger> core_logger_s;

public:
	static void init();
	
	inline static std::shared_ptr<spdlog::logger>& get_core_logger() { return core_logger_s; };
};
	
}

#define AITO_TRACE(...)	SPDLOG_LOGGER_TRACE(aito::Logger::get_core_logger(), __VA_ARGS__)
#define AITO_INFO(...)	SPDLOG_LOGGER_INFO(aito::Logger::get_core_logger(), __VA_ARGS__)
#define AITO_WARN(...)	SPDLOG_LOGGER_WARN(aito::Logger::get_core_logger(), __VA_ARGS__)
#define AITO_ERROR(...)	SPDLOG_LOGGER_ERROR(aito::Logger::get_core_logger(), __VA_ARGS__)
#define AITO_FATAL(...)	SPDLOG_LOGGER_CRITICAL(aito::Logger::get_core_logger(), __VA_ARGS__)
