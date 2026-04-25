#pragma once

#include <stdexcept>
#include <string>
#include <cstdint>

// Standard shell exit codes
enum ExitCode : std::uint8_t {
    SUCCESS = 0,              // Execution completed successfully
    GENERAL_FAILURE = 1,      // Catchall for general errors (e.g., cd to wrong dir)
    SYNTAX_ERROR = 2,         // Misuse of shell builtins or syntax error
    PERMISSION_DENIED = 126,  // Command invoked cannot execute (bad permissions)
    COMMAND_NOT_FOUND = 127,  // Command not found in PATH
    FATAL_SIGNAL_BASE = 128   // Base code for fatal signals (e.g., 128 + 2 for SIGINT)
};

// Base class for all shell-specific runtime failures
class YashError : public std::runtime_error {
public:
    explicit YashError(const std::string& message, int exit_code = 1)
        : std::runtime_error(message), exit_code_(exit_code) {
    }

    [[nodiscard]] int GetCode() const noexcept {
        return exit_code_;
    }

private:
    int exit_code_;
};

// System: critical failure (e.g., fork failed, pipe creation failed)
class YashSystemError final : public YashError {
public:
    explicit YashSystemError(const std::string& message)
        : YashError("yash: system error: " + message, ExitCode::GENERAL_FAILURE) {
    }
};

// Parser: syntax or tokenization failure
class YashSyntaxError final : public YashError {
public:
    explicit YashSyntaxError(const std::string& message)
        : YashError("yash: syntax error: " + message, ExitCode::SYNTAX_ERROR) {
    }
};

// Executor: command found but execution permission is denied
class YashPermissionError final : public YashError {
public:
    explicit YashPermissionError(const std::string& command)
        : YashError("yash: command found but permission denied: " + command,
                    ExitCode::PERMISSION_DENIED) {
    }
};

// Executor: executable file not found
class YashCommandNotFoundError final : public YashError {
public:
    explicit YashCommandNotFoundError(const std::string& command)
        : YashError("yash: command not found: " + command, ExitCode::COMMAND_NOT_FOUND) {
    }
};
