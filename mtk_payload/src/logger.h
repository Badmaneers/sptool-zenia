#pragma once
#include <cstdint>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdarg>

namespace mtk {

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERROR = 4
};

class Logger {
public:
    static Logger& instance();

    void set_level(LogLevel level);
    LogLevel level() const;

    void set_log_file(const std::string& path);

    void trace(const char* fmt, ...);
    void debug(const char* fmt, ...);
    void info(const char* fmt, ...);
    void warning(const char* fmt, ...);
    void error(const char* fmt, ...);

    void log_raw(const std::string& prefix, const uint8_t* data, size_t len);
    void log_hex(const std::string& prefix, const std::string& label, const uint8_t* data, size_t len);

private:
    Logger() = default;
    void log(LogLevel level, const char* fmt, va_list args);

    LogLevel level_ = LogLevel::INFO;
    std::ofstream log_file_;
};

// Convenience macros
#define LOG_TRACE(...) mtk::Logger::instance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) mtk::Logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...)  mtk::Logger::instance().info(__VA_ARGS__)
#define LOG_WARN(...)  mtk::Logger::instance().warning(__VA_ARGS__)
#define LOG_ERROR(...) mtk::Logger::instance().error(__VA_ARGS__)

} // namespace mtk
