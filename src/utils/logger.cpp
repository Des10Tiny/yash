#include "logger.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unistd.h>

bool Logger::Init(LogLevel level, const std::string& filename) {
    if (file.is_open()) {
        file.close();
    }

    current_level = level;
    if (level == LogLevel::NONE) {
        return true;
    }

    file.open(filename, std::ios::out | std::ios::app);

    if (!file.is_open()) {
        std::cerr << "yash warning: failed to open log file '" << filename << "'\n";
        return false;
    }

    file << "\n===============================================================\n"
         << "[" << GetCurrentTime() << "] [PID:" << getpid() << "] "
         << "YASH SESSION STARTED (Log Level: " << LevelToString(level) << ")\n"
         << "===============================================================\n";
    file.flush();

    return true;
}

void Logger::Write(LogLevel level, const std::string& msg) {
    if (!file.is_open() || level > current_level) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);

    file << "[" << GetCurrentTime() << "] "
         << "[PID:" << getpid() << "] "
         << "[" << LevelToString(level) << "] " << msg << "\n";

    file.flush();
}

std::string Logger::LevelToString(LogLevel level) {
    switch (level) {
    case LogLevel::FATAL: {
        return "FATAL";
    }
    case LogLevel::WARNING: {
        return "WARN";
    }
    case LogLevel::INFO: {
        return "INFO";
    }
    case LogLevel::DEBUG: {
        return "DEBUG";
    }
    default: {
        return "UNKNOWN";
    }
    }
}

std::string Logger::GetCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S");
    return ss.str();
}