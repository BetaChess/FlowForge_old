#pragma once
// Set the logger to log all, so flwfrg can decide what to log itself.
#define SPDLOG_ACTIVE_LEVEL 0


#include <memory>

#include <spdlog/spdlog.h>
#undef near
#undef far


namespace flwfrg
{

class Logger
{
private:
	static std::shared_ptr<spdlog::logger> core_logger_s;

public:
	static void init();

	inline static std::shared_ptr<spdlog::logger> &get_core_logger() { return core_logger_s; };
};

}// namespace flwfrg

#define FLOWFORGE_TRACE(...) SPDLOG_LOGGER_TRACE(flwfrg::Logger::get_core_logger(), __VA_ARGS__)
#define FLOWFORGE_INFO(...) SPDLOG_LOGGER_INFO(flwfrg::Logger::get_core_logger(), __VA_ARGS__)
#define FLOWFORGE_WARN(...) SPDLOG_LOGGER_WARN(flwfrg::Logger::get_core_logger(), __VA_ARGS__)
#define FLOWFORGE_ERROR(...) SPDLOG_LOGGER_ERROR(flwfrg::Logger::get_core_logger(), __VA_ARGS__)
#define FLOWFORGE_FATAL(...) SPDLOG_LOGGER_CRITICAL(flwfrg::Logger::get_core_logger(), __VA_ARGS__)
