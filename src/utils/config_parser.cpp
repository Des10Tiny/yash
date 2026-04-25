#include "config_parser.hpp"
#include <fstream>
#include <sstream>

ConfigParser::ConfigParser() {
    handlers_["loglevel"] = [](YashConfig& config, const std::string& value) {
        if (value == "debug") {
            config.log_level = LogLevel::DEBUG;
        } else if (value == "fatal") {
            config.log_level = LogLevel::FATAL;
        } else if (value == "warning") {
            config.log_level = LogLevel::WARNING;
        } else if (value == "info") {
            config.log_level = LogLevel::INFO;
        }
    };

    // TBA
}

YashConfig ConfigParser::Parse(const std::string& filename) {
    YashConfig config;
    std::ifstream file(filename);

    if (!file.is_open()) {
        return config;
    }

    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream is_line(line);
        std::string key;
        std::string value;

        if (std::getline(is_line, key, '=') && std::getline(is_line, value)) {

            auto it = handlers_.find(key);

            if (it != handlers_.end()) {
                it->second(config, value);
            } else if (key.starts_with("alias.")) {
                std::string alias_name = key.substr(6);
                config.aliases[alias_name] = value;
            } else {
                config.load_warnings.push_back("Unknown config key ignored: " + key);
            }
        }
    }
    return config;
}