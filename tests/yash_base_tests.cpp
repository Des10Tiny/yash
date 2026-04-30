#include "core/yash.hpp"
#include "utils/yash_error.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <string>

class YashTest : public ::testing::Test {
protected:
    int RunWithInput(const std::string& input_text, std::string& output_text) {
        std::stringstream fake_in(input_text);
        std::stringstream fake_out;

        auto* old_cin = std::cin.rdbuf(fake_in.rdbuf());
        auto* old_cout = std::cout.rdbuf(fake_out.rdbuf());

        int exit_code = 0;
        {
            Yash shell;
            try {
                exit_code = shell.Run();
            } catch (const YashExitException& e) {
                exit_code = e.GetCode();
            } catch (const std::exception& e) {
                exit_code = 1;
            }
        }
        std::cin.rdbuf(old_cin);
        std::cout.rdbuf(old_cout);

        std::cout.flush();
        output_text = fake_out.str();
        return exit_code;
    }
};

TEST_F(YashTest, ExitCommandTerminatesShell) {
    std::string output;
    int code = RunWithInput("exit\n", output);
    EXPECT_EQ(code, 0);
    EXPECT_TRUE(output.find("Bye!") != std::string::npos);
}

TEST_F(YashTest, EmptyInputDoesNotCrash) {
    std::string output;

    int code = RunWithInput("\n\n   \nexit\n", output);
    EXPECT_EQ(code, 0);

    int curr_pos = 0;
    for (int i = 0; i < 4; ++i) {
        curr_pos = output.find("yash>", curr_pos);
        EXPECT_TRUE(curr_pos != std::string::npos);
    }
}

TEST_F(YashTest, UnknownCommandPrintsError) {
    std::string output;

    int code = RunWithInput("some_random_garbage_command\nexit\n", output);
    EXPECT_EQ(code, 0);
}

TEST_F(YashTest, ExitWithCode) {
    std::string output;
    int code = RunWithInput("exit 42\n", output);

    EXPECT_EQ(code, 42);
}

TEST_F(YashTest, ConfigAutoGeneration) {
    std::filesystem::path temp_home = std::filesystem::temp_directory_path() / "yash_test_home";
    std::filesystem::create_directories(temp_home);

    std::string old_home = std::getenv("HOME") ? std::getenv("HOME") : "";
    setenv("HOME", temp_home.c_str(), 1);

    std::string output;
    RunWithInput("exit\n", output);

    std::filesystem::path expected_config = temp_home / ".config" / "yash" / "yash.conf";

    EXPECT_TRUE(output.find("created default config") != std::string::npos);
    EXPECT_TRUE(std::filesystem::exists(expected_config));

    setenv("HOME", old_home.c_str(), 1);
    std::filesystem::remove_all(temp_home);
}