#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include "logger.hpp"

struct YashConfig {
    LogLevel log_level = LogLevel::NONE;
    std::unordered_map<std::string, std::string> aliases;
    std::vector<std::string> load_warnings;
};

class ConfigParser {
public:
    ConfigParser();
    YashConfig Parse(const std::string& filename);

private:
    using ConfigHandler = std::function<void(YashConfig&, const std::string&)>;

    std::unordered_map<std::string, ConfigHandler> handlers_;
};