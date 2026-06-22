#include "core/yash.hpp"
#include "doctest.h"
#include "utils/mute_tests.hpp"
#include "utils/yash_error.hpp"

#include <iostream>
#include <sstream>
#include <string>

class YashTest {
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

TEST_CASE_FIXTURE(YashTest, "ExitCommandTerminatesShell") {
    std::string output;
    int code = RunWithInput("exit\n", output);
    CHECK(code == 0);
    CHECK(output.find("Bye!") != std::string::npos);
}

TEST_CASE_FIXTURE(YashTest, "EmptyInputDoesNotCrash") {
    std::string output;

    int code = RunWithInput("\n\n   \nexit\n", output);
    CHECK(code == 0);

    int curr_pos = 0;
    for (int i = 0; i < 4; ++i) {
        curr_pos = output.find("yash>", curr_pos);
        CHECK(curr_pos != std::string::npos);
    }
}

TEST_CASE_FIXTURE(YashTest, "UnknownCommandPrintsError") {
    MuteSTDERR mute;

    std::string output;

    int code = RunWithInput("some_random_garbage_command\nexit\n", output);
    CHECK(code == 0);
}

TEST_CASE_FIXTURE(YashTest, "ExitWithCode") {
    std::string output;
    int code = RunWithInput("exit 42\n", output);

    CHECK(code == 42);
}

TEST_CASE_FIXTURE(YashTest, "ConfigAutoGeneration") {
    std::filesystem::path temp_home = std::filesystem::temp_directory_path() / "yash_test_home";
    std::filesystem::remove_all(temp_home);
    std::filesystem::create_directories(temp_home);

    std::string old_home = std::getenv("HOME") ? std::getenv("HOME") : "";
    const char* old_xdg_ptr = std::getenv("XDG_CONFIG_HOME");
    std::string old_xdg = old_xdg_ptr ? old_xdg_ptr : "";
    bool had_xdg = old_xdg_ptr != nullptr;

    setenv("HOME", temp_home.c_str(), 1);
    unsetenv("XDG_CONFIG_HOME");

    std::string output;
    RunWithInput("exit\n", output);

    std::filesystem::path expected_config = temp_home / ".config" / "yash" / "yash.conf";

    CHECK(output.find("created default config") != std::string::npos);
    CHECK(std::filesystem::exists(expected_config));

    setenv("HOME", old_home.c_str(), 1);
    if (had_xdg) {
        setenv("XDG_CONFIG_HOME", old_xdg.c_str(), 1);
    }

    std::filesystem::remove_all(temp_home);
}

TEST_CASE_FIXTURE(YashTest, "E2E: Alias Pipeline Execution") {
    MuteSTDERR mute;

    std::string output;

    std::string input = "alias mycmd=\"true | false\"\nmycmd\nexit\n";

    int code = RunWithInput(input, output);

    CHECK(code == 0);
}