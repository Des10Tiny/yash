#include <iostream>
#include <string>
#include <sstream>
#include <csignal>
#include <cstdlib>
#include "tokenizer/tokenizer.hpp"
#include "parser/parser.hpp"

volatile std::sig_atomic_t g_signal_status = 0;

void SignalHandler(int signal) {
    g_signal_status = signal;
}

void PrintPipeline(const Pipeline& pipeline) {
    std::cout << "========================================\n";
    std::cout << "🌲 AST Pipeline (Commands: " << pipeline.commands.size() << ")\n";
    std::cout << "========================================\n";

    for (size_t i = 0; i < pipeline.commands.size(); ++i) {
        const auto& cmd = pipeline.commands[i];
        std::cout << "  [" << i + 1 << "] Command: ";

        for (const auto& arg : cmd.args) {
            std::cout << "\033[1;32m" << arg << "\033[0m ";
        }
        std::cout << "\n";

        if (!cmd.redirect_in.empty()) {
            std::cout << "      \033[1;34m< in:\033[0m  " << cmd.redirect_in << "\n";
        }

        if (!cmd.redirect_out.empty()) {
            std::cout << "      \033[1;33m> out:\033[0m " << cmd.redirect_out;

            if (cmd.append_out) {
                std::cout << " (append mode)";
            }

            std::cout << "\n";
        }
    }
    std::cout << "========================================\n";
}

int main() {
    std::signal(SIGINT, SignalHandler);

    std::cout << "yash (Parser AST Demo Mode)\n";
    std::cout << "Type 'exit' to quit or Ctrl+C to stop.\n";

    std::string line;
    while (true) {
        if (g_signal_status == SIGINT) {
            std::cout << "\nbye!\n";
            std::exit(0);
        }

        std::cout << "yash> ";
        if (!std::getline(std::cin, line) || line == "exit") {
            break;
        }

        if (line.empty()) {
            continue;
        }

        std::stringstream ss{line};
        try {
            Tokenizer tokenizer{&ss};
            Parser parser{tokenizer};

            if (auto pipeline = parser.ParsePipeline()) {
                PrintPipeline(*pipeline);
            }

        } catch (const std::exception& e) {
            std::cerr << "yash: " << e.what() << '\n';
        }
    }

    std::cout << "bye!\n";
    return 0;
}