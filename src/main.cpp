#include <iostream>
#include <string>
#include <sstream>
#include <csignal>
#include <cstdlib>
#include "tokenizer/tokenizer.hpp"

volatile std::sig_atomic_t g_signal_status = 0;

void SignalHandler(int signal) {
    g_signal_status = signal;
}

void PrintToken(const Token& t) {
    if (std::holds_alternative<WordToken>(t)) {
        std::cout << "Word: " << std::get<WordToken>(t).value << "\n";

    } else if (std::holds_alternative<PipeToken>(t)) {
        std::cout << "Pipe: |\n";

    } else if (std::holds_alternative<RedirectToken>(t)) {
        auto type = std::get<RedirectToken>(t);
        std::cout << "Redirect: " << static_cast<int>(type) << "\n";
    }
}

int main() {
    std::signal(SIGINT, SignalHandler);

    std::cout << "yash (Tokenize demo mode)\n";
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
            while (!tokenizer.IsEnd()) {
                PrintToken(tokenizer.GetToken());
                tokenizer.Next();
            }

        } catch (const std::exception& e) {
            std::cerr << "yash: syntax error: " << e.what() << '\n';
        }
    }

    std::cout << "bye!\n";
    return 0;
}