# Release v0.1.1: Interactive Demo & Fixes

### Added

- Interactive demo mode (`main.cpp`) to test tokenization in real-time.
- Graceful handling of `Ctrl+C` (SIGINT) in the REPL loop.

### Fixed

- Fixed a bug where the `Tokenizer` would silently drop the final token if it wasn't followed by a space or newline.
