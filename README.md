# yash (Yet Another Shell)

A lightweight, experimental terminal shell written in C++.

Currently in the **extreme Work-In-Progress (WIP)** stage. I am building this primarily to dive deeper into system programming, POSIX APIs (like `fork`, `execvp`, `pipe`), and C++ project architecture.

In the future, I might experiment with adding a plugin system or custom UI rendering (like `icat` in Kitty), but for now, it's just a personal learning sandbox.

## Features (Planned / Done)

- [x] Basic CI/CD, CMake setup, and Docker integration.
- [ ] Core REPL loop.
- [ ] Command tokenization (handling quotes, spaces).
- [ ] Parser & Abstract Syntax Tree (AST) generation.
- [ ] Execution of external commands.
- [ ] Pipes (`|`) and Redirections (`>`, `<`).

## Building from source

You need `cmake` and a C++20 compatible compiler (Clang/GCC). It builds seamlessly on both macOS and Linux.

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
./yash
```

## Running via Docker

If you just want to run the latest build in an isolated environment:

```sh
docker pull ghcr.io/des10tiny/yash:latest
docker run -it ghcr.io/des10tiny/yash:latest
```

## License

GPL-3.0
