#include "yash.hpp"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "parser/parser.hpp"
#include "tokenizer/tokenizer.hpp"
#include "utils/logger.hpp"
#include "utils/yash_error.hpp"

volatile std::sig_atomic_t g_signal_status = 0;

static void SignalHandler(int signal) {
    g_signal_status = signal;
}

Yash::Yash() {
    SetupSignals();

    std::filesystem::path config_dir = GetConfigDir();
    std::filesystem::path config_path = config_dir / "yash.conf";
    std::filesystem::path log_path = config_dir / "yash.log";

    try {
        if (!std::filesystem::exists(config_path)) {
            std::ofstream default_conf(config_path);

            if (default_conf.is_open()) {
                default_conf << "# Yash configuration file\n";
                default_conf << "loglevel=none\n";
                default_conf.close();
                std::cout << std::format(
                    "yash: created default config at {}\n", config_path.string()
                );
            } else {
                std::cerr << "yash: warning: failed to create default config(this is actually a "
                             "kind of unexpected configuration state for yash)\n";
            }
        }

        ConfigParser conf_parser;
        config_ = conf_parser.Parse(config_path.string());

    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << std::format("yash warning: config access denied ({})\n", e.what());
        std::cerr << "yash: falling back to default settings(this is actually a "
                     "kind of unexpected configuration state for yash)\n";
    }

    if (!Logger::Init(config_.log_level, log_path.string())) {
        std::cerr << "yash: logging system failed to start\n";
    } else {
        LOG_INFO("yash shell initialized successfully in {}", config_dir.string());
    }

    for (const auto& warning : config_.load_warnings) {
        LOG_WARN("{}", warning);
    }

    LOG_INFO("{} aliases loaded", config_.aliases.size());
    for (const auto& [key, value] : config_.aliases) {
        LOG_DEBUG("Loaded alias: {} -> {}", key, value);
    }

    LoadHistory();
}

Yash::~Yash() {
    SaveHistory();
    DisableRawTerminalMode();
    LOG_INFO("yash shell shutting down");
    SayGoodbye();
}

std::filesystem::path Yash::GetConfigDir() const {
    std::filesystem::path config_dir;
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");

    if (xdg_config && *xdg_config != '\0') {
        config_dir = std::filesystem::path(xdg_config) / "yash";
    } else {
        const char* home = std::getenv("HOME");

        if (home) {
            config_dir = std::filesystem::path(home) / ".config" / "yash";
        } else {
            config_dir = std::filesystem::current_path() / ".yash_config";
        }
    }

    if (!std::filesystem::exists(config_dir)) {
        std::filesystem::create_directories(config_dir);
    }

    return config_dir;
}

void Yash::SetupSignals() {
    std::signal(SIGINT, SignalHandler);
}

void Yash::PrintPrompt() const {
    std::cout << "yash> ";
}

int Yash::Run() {
    std::cout << "YASH\n";
    std::cout << "Type 'exit' to quit or Ctrl+C to stop.\n";

    std::string line;
    EnableRawTerminalMode();

    while (keep_running_) {
        if (g_signal_status == SIGINT) {
            keep_running_ = false;
            continue;
        }

        PrintPrompt();
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        ProcessLine(line);
    }

    return last_exit_status_;
}

void Yash::ProcessLine(const std::string& line) {
    LOG_DEBUG("User input: {}", line);
    std::stringstream ss{line};

    try {
        Tokenizer tokenizer{&ss};
        Parser parser{tokenizer};

        if (auto pipeline = parser.ParsePipeline()) {
            last_exit_status_ = executor_.RunPipeline(*pipeline);
        }

    } catch (const YashExitException& e) {
        last_exit_status_ = e.GetCode();
        throw;
    } catch (const YashError& e) {
        std::cerr << e.what() << '\n';
        last_exit_status_ = e.GetCode();

        switch (last_exit_status_) {
        case ExitCode::SYNTAX_ERROR:
            LOG_WARN("Syntax error: {}", e.what());
            break;
        case ExitCode::COMMAND_NOT_FOUND:
            LOG_WARN("Command not found: {}", e.what());
            break;
        case ExitCode::PERMISSION_DENIED:
            LOG_WARN("Permission denied: {}", e.what());
            break;
        default:
            LOG_WARN("Execution error/Builtin error: {}", e.what());
            break;
        }
    } catch (const std::exception& e) {
        std::cerr << "yash: unexpected fatal error: " << e.what() << '\n';
        last_exit_status_ = ExitCode::GENERAL_FAILURE;
        LOG_FATAL("Fatal exception: {}", e.what());
    }
}

void Yash::SayGoodbye() const {
    std::cout << "Bye!\n";
}

void Yash::EnableRawTerminalMode() {
}
void Yash::DisableRawTerminalMode() {
}
void Yash::LoadHistory() {
}
void Yash::SaveHistory() {
}
void Yash::RunInteractiveFuzzyFinder() {
}