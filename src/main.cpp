#include <iostream>
#include <string>
#include <sstream>
#include <csignal>
#include <cstdlib>

#include "tokenizer/tokenizer.hpp"
#include "parser/parser.hpp"
#include "utils/config_parser.hpp"
#include "utils/logger.hpp"
#include "utils/yash_error.hpp"

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

    ConfigParser conf_parser;
    YashConfig config = conf_parser.Parse(".yash.conf");

    Logger::Init(config.log_level, "yash.log");
    LOG_INFO("yash shell initialized successfully");

    for (const auto& warning : config.load_warnings) {
        LOG_WARN(warning);
    }

    LOG_INFO(std::to_string(config.aliases.size()) + " added alias");

    for (const auto& i : config.aliases) {
        std::string msg = std::string("Loaded alias ") + "key= " + i.first + ", value= " + i.second;
        LOG_DEBUG(msg);
    };

    std::cout << "yash (Parser AST Demo Mode)\n";
    std::cout << "Type 'exit' to quit or Ctrl+C to stop.\n";

    int last_exit_status = ExitCode::SUCCESS;

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

        LOG_DEBUG("User input: " + line);

        std::stringstream ss{line};
        try {
            Tokenizer tokenizer{&ss};
            Parser parser{tokenizer};

            if (auto pipeline = parser.ParsePipeline()) {
                PrintPipeline(*pipeline);
                last_exit_status = ExitCode::SUCCESS;
            }

        } catch (const YashSyntaxError& e) {
            std::cerr << e.what() << '\n';
            last_exit_status = e.GetCode();
            LOG_WARN(std::string(e.what()));

        } catch (const YashError& e) {
            std::cerr << e.what() << '\n';
            last_exit_status = e.GetCode();
            LOG_WARN(std::string("Execution error: ") + e.what());

        } catch (const std::exception& e) {
            std::cerr << "yash: unexpected fatal error: " << e.what() << '\n';
            last_exit_status = ExitCode::GENERAL_FAILURE;
            LOG_FATAL(std::string("Fatal exception: ") + e.what());
        }
    }

    LOG_INFO("yash shell shutting down");
    std::cout << "bye!\n";
    return last_exit_status;
}