#pragma once

#include "executor/executor.hpp"
#include "utils/config_parser.hpp"
#include <filesystem>
#include <replxx.hxx>
#include <string>

class Yash {
public:
    Yash();
    ~Yash();

    int Run();

private:
    [[nodiscard]] std::filesystem::path GetConfigDir() const;
    void SetupSignals();

    void ProcessLine(const std::string& line);
    void SayGoodbye() const;

    void EnableRawTerminalMode();
    void DisableRawTerminalMode();

    void LoadHistory();
    void SaveHistory();

    void RunInteractiveFuzzyFinder();

    YashConfig config_;
    Executor executor_;
    int last_exit_status_ = 0;
    bool keep_running_ = true;
    replxx::Replxx rx_;
};
