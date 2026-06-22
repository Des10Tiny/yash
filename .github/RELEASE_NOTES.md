# Release v0.6.0: The Alias Engine & Test Infrastructure Overhaul

## Summary

This release introduces a powerful, POSIX-compliant Alias Expansion Engine (`Expander`) and marks a complete overhaul of our testing infrastructure. We successfully decoupled the Tokenizer from the Parser by introducing an abstraction layer, allowing us to safely process aliases via lazy evaluation. Additionally, we migrated the entire test suite to `doctest` for faster, lighter, and more reliable testing, while fortifying the CI/CD environment compatibility.

## 🚀 Added

- **Alias Expansion Engine:** Implemented the `Expander` class, seamlessly integrated as an `ITokenizer` decorator. It dynamically evaluates command aliases, correctly identifying "command positions" and respecting boundaries like pipes (`|`) and I/O redirections (`>`, `<`).
- **Lazy Evaluation via Deque:** The `Expander` utilizes an internal `std::deque<Token>` buffer to expand aliases into mini-token streams on the fly. This avoids expensive string concatenations, protects quotes/escaping from being lost, and prevents OOM issues via zero-allocation moves (`std::move`).
- **Doctest Integration:** Completely replaced the heavy GTest framework with `doctest`. Tests are now significantly faster to compile and run.
- **VS Code TestMate Support:** Pre-configured workspace settings (`.vscode/settings.json`) to isolate test discovery per CMake preset, resolving duplicate test executions and UI clutter in the IDE.

## 🔧 Changed

- **Tokenizer/Parser Abstraction:** Abstracted the tokenizer into an `ITokenizer` interface. The Parser no longer depends on a concrete tokenizer, enabling endless possibilities for future stream decorators (e.g., Variable Expansion, Globbing).
- **Hardened System Tests:** E2E and integration tests handling POSIX file permissions now verify user privileges (`geteuid() == 0`). This prevents tests from falsely failing when executed in root environments (like Docker containers or CI runners) where kernel access checks are bypassed.
- **Safe Variant Handling:** Upgraded token processing to utilize modern C++17 `std::variant` patterns (`std::holds_alternative`, `std::get_if`), completely eliminating undefined behaviors related to empty aliases and ensuring strong type safety across the AST.
