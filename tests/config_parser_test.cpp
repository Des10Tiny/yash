#include <filesystem>
#include <fstream>

#include "doctest.h"
#include "utils/config_parser.hpp"

void CreateTestConfig(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    file << content;
    file.close();
}

class ConfigParserTest {
protected:
    const std::string test_filename = "test_yash.conf";

    ~ConfigParserTest() {
        std::error_code ec;
        std::filesystem::remove(test_filename, ec);
    }
};

TEST_CASE_FIXTURE(ConfigParserTest, "NoFileReturnsDefaults") {
    ConfigParser parser;
    YashConfig config = parser.Parse("non_existent_file.conf");

    CHECK(config.log_level == LogLevel::NONE);
    CHECK(config.aliases.empty());
}

TEST_CASE_FIXTURE(ConfigParserTest, "ParsesLogLevels") {
    ConfigParser parser;

    CreateTestConfig(test_filename, "loglevel=debug\n");
    CHECK(parser.Parse(test_filename).log_level == LogLevel::DEBUG);

    CreateTestConfig(test_filename, "loglevel=info\n");
    CHECK(parser.Parse(test_filename).log_level == LogLevel::INFO);

    CreateTestConfig(test_filename, "loglevel=warning\n");
    CHECK(parser.Parse(test_filename).log_level == LogLevel::WARNING);

    CreateTestConfig(test_filename, "loglevel=fatal\n");
    CHECK(parser.Parse(test_filename).log_level == LogLevel::FATAL);
}

TEST_CASE_FIXTURE(ConfigParserTest, "IgnoresCommentsAndEmptyLines") {
    std::string content = "# This is a comment\n"
                          "\n"
                          "loglevel=info\n"
                          "# Another comment\n"
                          "   \n"
                          "alias.ll=ls -la\n";

    CreateTestConfig(test_filename, content);
    ConfigParser parser;
    YashConfig config = parser.Parse(test_filename);

    CHECK(config.log_level == LogLevel::INFO);
    CHECK(config.aliases.size() == 1);
    CHECK(config.aliases["ll"] == "ls -la");
}

TEST_CASE_FIXTURE(ConfigParserTest, "ParsesAliases") {
    std::string content = "alias.ll=ls -la\n"
                          "alias.go=cd\n"
                          "alias.g=git status\n";

    CreateTestConfig(test_filename, content);
    ConfigParser parser;
    YashConfig config = parser.Parse(test_filename);

    CHECK(config.aliases.size() == 3);
    CHECK(config.aliases["ll"] == "ls -la");
    CHECK(config.aliases["go"] == "cd");
    CHECK(config.aliases["g"] == "git status");
}

TEST_CASE_FIXTURE(ConfigParserTest, "IgnoresUnknownKeysGracefully") {
    std::string content = "loglevel=debug\n"
                          "some_weird_setting=42\n"
                          "color=red\n";

    CreateTestConfig(test_filename, content);
    ConfigParser parser;

    CHECK_NOTHROW({
        YashConfig config = parser.Parse(test_filename);
        CHECK(config.log_level == LogLevel::DEBUG);
        CHECK(config.aliases.empty());
    });
}

TEST_CASE_FIXTURE(ConfigParserTest, "CollectsWarningsForUnknownKeys") {
    std::string content = "loglevel=debug\n"
                          "some_weird_setting=42\n"
                          "another_typo=1\n";

    CreateTestConfig(test_filename, content);
    ConfigParser parser;
    YashConfig config = parser.Parse(test_filename);

    CHECK(config.log_level == LogLevel::DEBUG);
    CHECK(config.load_warnings.size() == 2);
}