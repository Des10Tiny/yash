# Release v0.5.0: I/O Redirection, Builtins & Core Architecture

## Summary

This release marks a massive architectural shift for `yash`. The monolithic `main.cpp` has been completely dismantled and rebuilt into a robust, object-oriented `Yash` application class. Alongside the architectural overhaul, the Execution Engine has been expanded to support full POSIX I/O redirections (`>`, `<`, `>>`) and essential shell builtins (`cd`, `exit`). We also heavily reinforced the safety of system calls using strict RAII patterns for file descriptors to guarantee leak-free process forking.

## 🚀 Added

- **I/O Redirections:** The `Executor` now natively handles standard input/output redirections (`>`, `<`, `>>`). It accurately maps bitwise flags (e.g., `O_TRUNC`, `O_APPEND`) and safely manages POSIX permissions during file creation.
- **Core Builtins:** Implemented the first native shell builtins: `cd` for directory navigation and `exit` for graceful termination (throwing a structured `YashExitException` to unwind the stack safely).
- **Object-Oriented Shell Architecture:** Encapsulated the entire REPL lifecycle, signal handling, and state management into the `Yash` class. `main.cpp` is now a minimal 20-line entry point dedicated solely to bootstrapping and fatal error catching.
- **XDG Base Directory Compliance:** `yash` now adheres to modern Unix standards, automatically generating and loading its configuration and logs from `~/.config/yash/` instead of cluttering the user's home directory.

## 🔧 Changed

- **Strict RAII for File Descriptors:** Introduced the `UniqueFD` wrapper alongside the existing `ScopedPipe`. File descriptors are now guaranteed to close when going out of scope.
- **Fork-Safe Error Handling:** Enforced strict `noexcept` and non-throwing paradigms inside child processes (`pid == 0`). System call failures (like `open()` or `dup2()`) inside a fork now safely trigger `_Exit(1)` instead of throwing C++ exceptions, preventing catastrophic `std::terminate` crashes.
- **Logger Resilience:** The logging system now gracefully handles `LogLevel::NONE` without creating empty files on the disk, and safely recovers if it lacks permissions to write to the config directory.
- **Integration Testing Isolation:** Upgraded the GTest suite to intercept `std::cin` / `std::cout` for full-app integration tests. Implemented environment variable spoofing (`setenv("HOME")`) to ensure config tests run in isolated temporary directories without overwriting the developer's actual configs.
