#include "logger.h"
#include <cstdarg>
#include <cstdio>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace mtk {

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::set_level(LogLevel level) {
    level_ = level;
}

LogLevel Logger::level() const {
    return level_;
}

void Logger::set_log_file(const std::string& path) {
    log_file_.open(path, std::ios::out | std::ios::trunc);
}

void Logger::trace(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(LogLevel::TRACE, fmt, args);
    va_end(args);
}

void Logger::debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(LogLevel::DEBUG, fmt, args);
    va_end(args);
}

void Logger::info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(LogLevel::INFO, fmt, args);
    va_end(args);
}

void Logger::warning(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(LogLevel::WARNING, fmt, args);
    va_end(args);
}

void Logger::error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log(LogLevel::ERROR, fmt, args);
    va_end(args);
}

void Logger::log_raw(const std::string& prefix, const uint8_t* data, size_t len) {
    if (level_ > LogLevel::DEBUG) return;

    std::ostringstream oss;
    oss << prefix;
    for (size_t i = 0; i < len && i < 64; ++i) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", data[i]);
        oss << buf;
    }
    if (len > 64) oss << "...";

    std::string msg = oss.str();
    if (log_file_.is_open()) {
        log_file_ << msg << "\n";
        log_file_.flush();
    }
    std::cerr << msg << "\n";
}

void Logger::log_hex(const std::string& prefix, const std::string& label, const uint8_t* data, size_t len) {
    if (level_ > LogLevel::DEBUG) return;

    std::ostringstream oss;
    oss << prefix << " " << label << " (" << len << " bytes): ";
    for (size_t i = 0; i < len; ++i) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", data[i]);
        oss << buf;
    }

    std::string msg = oss.str();
    if (log_file_.is_open()) {
        log_file_ << msg << "\n";
        log_file_.flush();
    }
    std::cerr << msg << "\n";
}

void Logger::log(LogLevel level, const char* fmt, va_list args) {
    if (level < level_) return;

    const char* level_str = "";
    switch (level) {
        case LogLevel::TRACE:   level_str = "TRACE"; break;
        case LogLevel::DEBUG:   level_str = "DEBUG"; break;
        case LogLevel::INFO:    level_str = "INFO"; break;
        case LogLevel::WARNING: level_str = "WARN"; break;
        case LogLevel::ERROR:   level_str = "ERROR"; break;
    }

    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%H:%M:%S")
        << " " << level_str << ": " << buf;

    std::string msg = oss.str();
    if (log_file_.is_open()) {
        log_file_ << msg << "\n";
        log_file_.flush();
    }
    std::cerr << msg << "\n";
}

} // namespace mtk
