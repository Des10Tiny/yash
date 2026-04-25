# Release v0.3.0: Core Infrastructure, Logging & Configuration

## Summary

Built the foundational system infrastructure required for the upcoming execution engine. This release introduces centralized error handling, safe OS resource management, and an extensible configuration system to make `yash` robust and production-ready.

## 🚀 Added

- **Configuration System:** Implemented `ConfigParser` to read settings from `~/.yash.conf`. Currently supports dynamic `loglevel` and has a scalable architecture for future aliases and UI settings.
- **System Logger:** Added a thread-safe, static `Logger` class. It safely records runtime events, warnings, and PIDs to `yash.log` without polluting the user's terminal.
- **Resource Management (RAII):** Introduced `ScopedFD`, a secure C++ wrapper for POSIX file descriptors to prevent pipe leaks during `fork()` and `exec()` operations.
- **Custom Exception Hierarchy:** Replaced generic `std::runtime_error` with POSIX-compliant custom exceptions (`YashError`, `YashSyntaxError`, `YashSystemError`, etc.) for accurate exit status codes (0, 1, 2, 127).
- Added comprehensive GTest suite for the configuration parser and RAII wrappers.

## 🔧 Changed

- **REPL Refactoring:** `main.cpp` now correctly initializes the logging and configuration subsystems before starting the interactive loop.
- **Error Propagation:** The parser and tokenizer now throw specific `YashSyntaxError` exceptions instead of generic runtime errors, improving debuggability.
- Log files now use append mode (`std::ios::app`) with clear visual session dividers instead of overwriting previous crash data.
