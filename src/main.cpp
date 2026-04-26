#include <iostream>
#include <string>
#include <sstream>
#include <csignal>
#include <cstdlib>

#include "tokenizer/tokenizer.hpp"
#include "parser/parser.hpp"
#include "executor/executor.hpp"
#include "utils/config_parser.hpp"
#include "utils/logger.hpp"
#include "utils/yash_error.hpp"

volatile std::sig_atomic_t g_signal_status = 0;

void SignalHandler(int signal) {
    g_signal_status = signal;
}

int main() {
    std::signal(SIGINT, SignalHandler);

    ConfigParser conf_parser;
    YashConfig config = conf_parser.Parse(".yash.conf");

    Logger::Init(config.log_level, ".yash.log");
    LOG_INFO("yash shell initialized successfully");

    for (const auto& warning : config.load_warnings) {
        LOG_WARN(warning);
    }

    LOG_INFO(std::to_string(config.aliases.size()) + " aliases loaded");

    for (const auto& [key, value] : config.aliases) {
        LOG_DEBUG("Loaded alias: " + key + " -> " += value);
    }

    Executor executor;

    std::cout << "yash (Execution Mode)\n";
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
                last_exit_status = executor.RunPipeline(*pipeline);
            }

        } catch (const YashSyntaxError& e) {
            std::cerr << e.what() << '\n';

            last_exit_status = e.GetCode();
            LOG_WARN(std::string("Syntax error: ") + e.what());

        } catch (const YashCommandNotFoundError& e) {
            std::cerr << e.what() << '\n';

            last_exit_status = e.GetCode();
            LOG_WARN(std::string("Command not found: ") + e.what());

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