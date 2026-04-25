#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>

#include "utils/config_parser.hpp"

void CreateTestConfig(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    file << content;
    file.close();
}

class ConfigParserTest : public ::testing::Test {
protected:
    const std::string test_filename = "test_yash.conf";

    void TearDown() override {
        std::remove(test_filename.c_str());
    }
};

TEST_F(ConfigParserTest, NoFileReturnsDefaults) {
    ConfigParser parser;
    YashConfig config = parser.Parse("non_existent_file.conf");

    EXPECT_EQ(config.log_level, LogLevel::NONE);
    EXPECT_TRUE(config.aliases.empty());
}

TEST_F(ConfigParserTest, ParsesLogLevels) {
    ConfigParser parser;

    CreateTestConfig(test_filename, "loglevel=debug\n");
    EXPECT_EQ(parser.Parse(test_filename).log_level, LogLevel::DEBUG);

    CreateTestConfig(test_filename, "loglevel=info\n");
    EXPECT_EQ(parser.Parse(test_filename).log_level, LogLevel::INFO);

    CreateTestConfig(test_filename, "loglevel=warning\n");
    EXPECT_EQ(parser.Parse(test_filename).log_level, LogLevel::WARNING);

    CreateTestConfig(test_filename, "loglevel=fatal\n");
    EXPECT_EQ(parser.Parse(test_filename).log_level, LogLevel::FATAL);
}

TEST_F(ConfigParserTest, IgnoresCommentsAndEmptyLines) {
    std::string content =
        "# This is a comment\n"
        "\n"
        "loglevel=info\n"
        "# Another comment\n"
        "   \n"
        "alias.ll=ls -la\n";

    CreateTestConfig(test_filename, content);
    ConfigParser parser;
    YashConfig config = parser.Parse(test_filename);

    EXPECT_EQ(config.log_level, LogLevel::INFO);
    EXPECT_EQ(config.aliases.size(), 1);
    EXPECT_EQ(config.aliases["ll"], "ls -la");
}

TEST_F(ConfigParserTest, ParsesAliases) {
    std::string content =
        "alias.ll=ls -la\n"
        "alias.go=cd\n"
        "alias.g=git status\n";

    CreateTestConfig(test_filename, content);
    ConfigParser parser;
    YashConfig config = parser.Parse(test_filename);

    EXPECT_EQ(config.aliases.size(), 3);
    EXPECT_EQ(config.aliases["ll"], "ls -la");
    EXPECT_EQ(config.aliases["go"], "cd");
    EXPECT_EQ(config.aliases["g"], "git status");
}

TEST_F(ConfigParserTest, IgnoresUnknownKeysGracefully) {
    std::string content =
        "loglevel=debug\n"
        "some_weird_setting=42\n"
        "color=red\n";

    CreateTestConfig(test_filename, content);
    ConfigParser parser;

    EXPECT_NO_THROW({
        YashConfig config = parser.Parse(test_filename);
        EXPECT_EQ(config.log_level, LogLevel::DEBUG);
        EXPECT_TRUE(config.aliases.empty());
    });
}

TEST_F(ConfigParserTest, CollectsWarningsForUnknownKeys) {
    std::string content =
        "loglevel=debug\n"
        "some_weird_setting=42\n"
        "another_typo=1\n";

    CreateTestConfig(test_filename, content);
    ConfigParser parser;
    YashConfig config = parser.Parse(test_filename);

    EXPECT_EQ(config.log_level, LogLevel::DEBUG);
    EXPECT_EQ(config.load_warnings.size(), 2);
}