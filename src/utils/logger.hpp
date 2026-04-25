#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <cstdint>

enum class LogLevel : std::uint8_t { NONE = 0, FATAL = 1, WARNING = 2, INFO = 3, DEBUG = 4 };

class Logger {
public:
    static void Init(LogLevel level, const std::string& filename = ".yash.log");
    static void Write(LogLevel level, const std::string& msg);

    static LogLevel GetLevel() {
        return current_level;
    }

private:
    static std::string GetCurrentTime();
    static std::string LevelToString(LogLevel level);

    inline static LogLevel current_level = LogLevel::NONE;
    inline static std::ofstream file;
    inline static std::mutex mutex;
};

#define LOG_DEBUG(msg)                             \
    do {                                           \
        if (Logger::GetLevel() >= LogLevel::DEBUG) \
            Logger::Write(LogLevel::DEBUG, msg);   \
    } while (0)

#define LOG_FATAL(msg)                             \
    do {                                           \
        if (Logger::GetLevel() >= LogLevel::FATAL) \
            Logger::Write(LogLevel::FATAL, msg);   \
    } while (0)

#define LOG_WARN(msg)                                \
    do {                                             \
        if (Logger::GetLevel() >= LogLevel::WARNING) \
            Logger::Write(LogLevel::WARNING, msg);   \
    } while (0)

#define LOG_INFO(msg)                             \
    do {                                          \
        if (Logger::GetLevel() >= LogLevel::INFO) \
            Logger::Write(LogLevel::INFO, msg);   \
    } while (0)
