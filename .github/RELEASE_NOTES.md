# Release v0.2.0: Add parser & Interactive Demo

## Summary

Introduced the core parsing engine for `yash`. The shell can now convert raw tokens into an Abstract Syntax Tree (AST) ready for execution.

## 🚀 Added

- Implemented a Recursive Descent Parser (`Parser` class) to build structured `Pipeline` and `Command` objects.
- Added full support for parsing Input/Output redirections (`<`, `>`, `>>`).
- Added robust syntax validation (e.g., catching dangling pipes or missing files after redirects).
- Added an interactive AST Visualizer in the REPL loop.
- Comprehensive GTest suite covering standard commands, pipelines, edge cases, and syntax errors.

## 🔧 Changed

- Refactored `main.cpp` to pipe the Tokenizer output directly into the Parser.
