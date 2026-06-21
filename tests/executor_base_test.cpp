#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

#include "doctest.h"
#include "executor/executor.hpp"
#include "parser/parser.hpp"
#include "utils/mute_tests.hpp"
#include "utils/yash_error.hpp"

Pipeline MakePipeline(const std::vector<std::vector<std::string>>& cmds_args) {
    Pipeline p;

    for (const auto& args : cmds_args) {
        Command cmd;
        cmd.args = args;
        p.commands.push_back(cmd);
    }

    return p;
}

class ExecutorTest {
protected:
    Executor executor;

    const std::string no_exec_file = "/tmp/yash_test_no_exec.sh";

    ExecutorTest() {
        std::ofstream f(no_exec_file);
        f << "#!/bin/sh\necho fail\n";
        f.close();

        chmod(no_exec_file.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    }

    ~ExecutorTest() {
        std::remove(no_exec_file.c_str());
    }
};

TEST_CASE_FIXTURE(ExecutorTest, "EmptyPipelineReturnsZero") {
    Pipeline p;
    CHECK(executor.RunPipeline(p) == 0);
}

TEST_CASE_FIXTURE(ExecutorTest, "EmptyCommandArgsReturnsZero") {
    Pipeline p;
    Command empty_cmd;
    p.commands.push_back(empty_cmd);
    CHECK(executor.RunPipeline(p) == 0);
}

TEST_CASE_FIXTURE(ExecutorTest, "ExternalCommandSuccess") {
    auto p = MakePipeline({{"true"}});
    CHECK(executor.RunPipeline(p) == 0);
}

TEST_CASE_FIXTURE(ExecutorTest, "ExternalCommandFailure") {
    auto p = MakePipeline({{"false"}});
    CHECK(executor.RunPipeline(p) == 1);
}

TEST_CASE_FIXTURE(ExecutorTest, "CommandNotFoundReturns127") {
    MuteSTDERR mute;

    auto p = MakePipeline({{"nonexistent_command_12345"}});
    CHECK(executor.RunPipeline(p) == ExitCode::COMMAND_NOT_FOUND);
}

TEST_CASE_FIXTURE(ExecutorTest, "PermissionDeniedReturns126") {
    MuteSTDERR mute;

    auto p = MakePipeline({{no_exec_file}});
    CHECK(executor.RunPipeline(p) == ExitCode::PERMISSION_DENIED);
}

TEST_CASE_FIXTURE(ExecutorTest, "SimplePipelineTwoCommands") {
    MuteAllSTD mute;

    auto p = MakePipeline({{"echo", "hello"}, {"grep", "hello"}});
    CHECK(executor.RunPipeline(p) == 0);
}

TEST_CASE_FIXTURE(ExecutorTest, "PipelineWithFailureAtTheEnd") {
    auto p = MakePipeline({{"echo", "test"}, {"false"}});
    CHECK(executor.RunPipeline(p) == 1);
}

TEST_CASE_FIXTURE(ExecutorTest, "PipelineWithCommandNotFoundAtTheEnd") {
    MuteSTDERR mute;

    auto p = MakePipeline({{"echo", "test"}, {"not_exists"}});
    CHECK(executor.RunPipeline(p) == ExitCode::COMMAND_NOT_FOUND);
}

TEST_CASE_FIXTURE(ExecutorTest, "LongPipeline") {
    MuteAllSTD mute;

    auto p = MakePipeline({{"echo", "hello"}, {"cat"}, {"cat"}, {"cat"}, {"grep", "hello"}});
    CHECK(executor.RunPipeline(p) == 0);
}

TEST_CASE_FIXTURE(ExecutorTest, "FailingCommandInTheMiddle") {
    auto p = MakePipeline({{"false"}, {"true"}});
    CHECK(executor.RunPipeline(p) == 0);
}

TEST_CASE_FIXTURE(ExecutorTest, "MassiveOutputDoesNotDeadlock") {
    MuteAllSTD mute;

    auto p = MakePipeline({{"yes", "test"}, {"head", "-n", "100"}});
    CHECK(executor.RunPipeline(p) == 0);
}

TEST_CASE_FIXTURE(ExecutorTest, "DirectoryExecutionAttemptReturns126") {
    MuteSTDERR mute;

    auto p = MakePipeline({{"/"}});
    CHECK(executor.RunPipeline(p) == ExitCode::PERMISSION_DENIED);
}

TEST_CASE_FIXTURE(ExecutorTest, "KilledBySignalReturns128PlusSignal") {
    auto p = MakePipeline({{"sh", "-c", "kill -9 $$"}});
    CHECK(executor.RunPipeline(p) == 137);
}

TEST_CASE_FIXTURE(ExecutorTest, "MassiveArgumentsAllocation") {
    MuteAllSTD mute;

    std::vector<std::string> args = {"echo"};

    for (int i = 0; i < 10000; ++i) {
        args.push_back("stress_arg");
    }
    auto p = MakePipeline({args});

    CHECK(executor.RunPipeline(p) == 0);
}

TEST_CASE_FIXTURE(ExecutorTest, "FileDescriptorLeakStressTest_100_Pipes") {
    MuteAllSTD mute;

    std::vector<std::vector<std::string>> commands;
    commands.push_back({"echo", "stress_test"});

    for (int i = 0; i < 98; ++i) {
        commands.push_back({"cat"});
    }
    commands.push_back({"grep", "stress_test"});

    auto p = MakePipeline(commands);
    CHECK(executor.RunPipeline(p) == 0);
}

TEST_CASE_FIXTURE(ExecutorTest, "EmptyCommandInTheMiddleOfPipeline") {
    Pipeline p;
    Command cmd1, cmd2, cmd3;
    cmd1.args = {"echo", "hi"};
    cmd2.args = {};
    cmd3.args = {"cat"};

    p.commands.push_back(cmd1);
    p.commands.push_back(cmd2);
    p.commands.push_back(cmd3);

    int status = executor.RunPipeline(p);
    CHECK(status == 0);
}

TEST_CASE_FIXTURE(ExecutorTest, "MachineGun_1000_CommandsSpeedTest") {
    for (int i = 0; i < 1000; ++i) {
        auto p = MakePipeline({{"true"}});
        CHECK(executor.RunPipeline(p) == 0);
    }
}

TEST_CASE_FIXTURE(ExecutorTest, "IdioticInput_MaxArgsLimit") {
    MuteSTDERR mute;

    std::vector<std::string> args = {"echo"};
    for (int i = 0; i < 50000; ++i) {
        args.push_back("NASTY_TEST_ARGUMENT");
    }
    auto p = MakePipeline({args});

    executor.RunPipeline(p);
}

// ============================================================================
// BUILTINS
// ============================================================================

TEST_CASE_FIXTURE(ExecutorTest, "BuiltinCd_ChangesDirectorySuccessfully") {
    namespace fs = std::filesystem;

    fs::path original_dir = fs::current_path();

    auto p = MakePipeline({{"cd", "/tmp"}});
    CHECK(executor.RunPipeline(p) == 0);

    fs::path new_dir = fs::current_path();

    CHECK(fs::equivalent(new_dir, fs::path("/tmp")));

    fs::current_path(original_dir);
}

TEST_CASE_FIXTURE(ExecutorTest, "BuiltinCd_ReturnsErrorOnNonExistentPath") {
    auto p = MakePipeline({{"cd", "/path/that/definitely/does/not/exist_12345"}});

    try {
        executor.RunPipeline(p);
        FAIL("Expected YashBuiltinError to be thrown");
    } catch (const YashBuiltinError& e) {
        CHECK(e.GetCode() == 1);
        CHECK(std::string(e.what()).find("No such file or directory") != std::string::npos);
    }
}

TEST_CASE_FIXTURE(ExecutorTest, "BuiltinCd_NoArgsGoesToHome") {
    namespace fs = std::filesystem;
    fs::path original_dir = fs::current_path();

    auto p = MakePipeline({{"cd"}});
    CHECK(executor.RunPipeline(p) == 0);

    const char* home = getenv("HOME");

    if (home) {
        fs::path new_dir = fs::current_path();
        CHECK(fs::equivalent(new_dir, fs::path(home)));
    }

    fs::current_path(original_dir);
}

TEST_CASE_FIXTURE(ExecutorTest, "BuiltinExit_ThrowsExitException") {
    auto p = MakePipeline({{"exit"}});

    CHECK_THROWS_AS({ executor.RunPipeline(p); }, YashExitException);
}

TEST_CASE_FIXTURE(ExecutorTest, "BuiltinExit_DoesNotRunSubsequentCommandsInPipeline") {
    auto p = MakePipeline({{"exit"}, {"echo", "should_not_run"}});

    CHECK_THROWS_AS({ executor.RunPipeline(p); }, YashBuiltinError);
}

TEST_CASE_FIXTURE(ExecutorTest, "BuiltinExit_WithValidCodeThrowsThatCode") {
    auto p = MakePipeline({{"exit", "42"}});

    try {
        executor.RunPipeline(p);
        FAIL("Expected YashExitException");
    } catch (const YashExitException& e) {
        CHECK(e.GetCode() == 42);
    }
}

TEST_CASE_FIXTURE(ExecutorTest, "BuiltinExit_WithInvalidAlphaCodeThrowsBuiltinError") {
    auto p = MakePipeline({{"exit", "abc"}});

    try {
        executor.RunPipeline(p);
        FAIL("Expected YashBuiltinError");
    } catch (const YashBuiltinError& e) {
        CHECK(e.GetCode() == 1);
        CHECK(std::string(e.what()).find("numeric argument required") != std::string::npos);
    }
}

TEST_CASE_FIXTURE(ExecutorTest, "BuiltinExit_TooManyArgsThrowsBuiltinError") {
    auto p = MakePipeline({{"exit", "1", "2"}});

    try {
        executor.RunPipeline(p);
        FAIL("Expected YashBuiltinError");
    } catch (const YashBuiltinError& e) {
        CHECK(e.GetCode() == 1);
        CHECK(std::string(e.what()).find("too many arguments") != std::string::npos);
    }
}