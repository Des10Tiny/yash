# Release v0.4.0: Execution Engine & Process Pipelining

## Summary

This is a major milestone for `yash`. Transitioning from a purely analytical AST parser, this release introduces a fully functional Execution Engine. `yash` can now natively interact with the UNIX kernel to spawn processes, manage execution environments, and chain complex pipelines without resource leaks or deadlocks.

## 🚀 Added

- **Execution Engine:** Implemented the `Executor` class responsible for traversing the AST and executing external binaries via `fork()` and `execvp()`.
- **Relay-Race Pipelining:** Engineered a highly optimized pipeline architecture (`ls | cat | grep`) that maintains only two active file descriptors at any time. This prevents FD exhaustion and deadlocks, allowing pipelines of infinite length.
- **POSIX Exit Codes Mapping:** Implemented accurate translation of kernel signals and process exit statuses (e.g., `127` for Command Not Found, `126` for Permission Denied, `128+N` for fatal signals) using `waitpid` bitwise macros.
- **Stress & Load Testing:** Added an aggressive testing suite that runs pipeline stress tests (100+ chained pipes) and speed execution tests (1000 consecutive commands).

## 🔧 Changed

- **REPL Integration:** Replaced the AST demo output in `main.cpp` with active command execution. The shell now acts as a true interactive prompt.
- **Child Process Safety:** Enforced `_Exit()` calls within forked processes to prevent C++ exception leaks and "zombie shell" cloning during execution failures.
- **Sanitizer Infrastructure:** Overhauled the `CMakeLists.txt` build system to support seamless CI/CD integration with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan), guaranteeing(but that's not for sure, it's c++) zero memory or descriptor leaks during execution.
