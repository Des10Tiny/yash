#pragma once

#include <cstdint>
#include <format>
#include <fstream>
#include <mutex>
#include <string>

enum class LogLevel : std::uint8_t { NONE = 0, FATAL = 1, WARNING = 2, INFO = 3, DEBUG = 4 };

class Logger {
public:
    [[nodiscard("You must verify if Logger was actually created before using it")]] static bool
    Init(LogLevel level, const std::string& filename = ".yash.log");
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

#ifdef NDEBUG
#define LOG_DEBUG(fmt_str, ...)                                                                    \
    do {                                                                                           \
    } while (0)
#else
#define LOG_DEBUG(fmt_str, ...)                                                                    \
    do {                                                                                           \
        if (Logger::GetLevel() >= LogLevel::DEBUG)                                                 \
            Logger::Write(LogLevel::DEBUG, std::format(fmt_str __VA_OPT__(, ) __VA_ARGS__));       \
    } while (0)
#endif

#define LOG_FATAL(fmt_str, ...)                                                                    \
    do {                                                                                           \
        if (Logger::GetLevel() >= LogLevel::FATAL)                                                 \
            Logger::Write(LogLevel::FATAL, std::format(fmt_str __VA_OPT__(, ) __VA_ARGS__));       \
    } while (0)

#define LOG_WARN(fmt_str, ...)                                                                     \
    do {                                                                                           \
        if (Logger::GetLevel() >= LogLevel::WARNING)                                               \
            Logger::Write(LogLevel::WARNING, std::format(fmt_str __VA_OPT__(, ) __VA_ARGS__));     \
    } while (0)

#define LOG_INFO(fmt_str, ...)                                                                     \
    do {                                                                                           \
        if (Logger::GetLevel() >= LogLevel::INFO)                                                  \
            Logger::Write(LogLevel::INFO, std::format(fmt_str __VA_OPT__(, ) __VA_ARGS__));        \
    } while (0)
