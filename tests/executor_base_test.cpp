#include <gtest/gtest.h>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

#include "executor/executor.hpp"
#include "parser/parser.hpp"
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

class ExecutorTest : public ::testing::Test {
protected:
    Executor executor;

    const std::string no_exec_file = "/tmp/yash_test_no_exec.sh";

    void SetUp() override {
        std::ofstream f(no_exec_file);
        f << "#!/bin/sh\necho fail\n";
        f.close();

        chmod(no_exec_file.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    }

    void TearDown() override {
        std::remove(no_exec_file.c_str());
    }
};

TEST_F(ExecutorTest, EmptyPipelineReturnsZero) {
    Pipeline p;
    EXPECT_EQ(executor.RunPipeline(p), 0);
}

TEST_F(ExecutorTest, EmptyCommandArgsReturnsZero) {
    Pipeline p;
    Command empty_cmd;
    p.commands.push_back(empty_cmd);
    EXPECT_EQ(executor.RunPipeline(p), 0);
}

TEST_F(ExecutorTest, RunsBuiltinSingleCommand) {
    auto p = MakePipeline({{"cd", "/tmp"}});
    EXPECT_EQ(executor.RunPipeline(p), 1);
}

TEST_F(ExecutorTest, ExternalCommandSuccess) {
    auto p = MakePipeline({{"true"}});
    EXPECT_EQ(executor.RunPipeline(p), 0);
}

TEST_F(ExecutorTest, ExternalCommandFailure) {
    auto p = MakePipeline({{"false"}});
    EXPECT_EQ(executor.RunPipeline(p), 1);
}

TEST_F(ExecutorTest, CommandNotFoundReturns127) {
    auto p = MakePipeline({{"nonexistent_command_12345"}});
    EXPECT_EQ(executor.RunPipeline(p), ExitCode::COMMAND_NOT_FOUND);
}

TEST_F(ExecutorTest, PermissionDeniedReturns126) {
    auto p = MakePipeline({{no_exec_file}});
    EXPECT_EQ(executor.RunPipeline(p), ExitCode::PERMISSION_DENIED);
}

TEST_F(ExecutorTest, SimplePipelineTwoCommands) {
    auto p = MakePipeline({{"echo", "hello"}, {"grep", "hello"}});
    EXPECT_EQ(executor.RunPipeline(p), 0);
}

TEST_F(ExecutorTest, PipelineWithFailureAtTheEnd) {
    auto p = MakePipeline({{"echo", "test"}, {"false"}});
    EXPECT_EQ(executor.RunPipeline(p), 1);
}

TEST_F(ExecutorTest, PipelineWithCommandNotFoundAtTheEnd) {
    auto p = MakePipeline({{"echo", "test"}, {"not_exists"}});
    EXPECT_EQ(executor.RunPipeline(p), ExitCode::COMMAND_NOT_FOUND);
}

TEST_F(ExecutorTest, LongPipeline) {
    auto p = MakePipeline({{"echo", "hello"}, {"cat"}, {"cat"}, {"cat"}, {"grep", "hello"}});
    EXPECT_EQ(executor.RunPipeline(p), 0);
}

TEST_F(ExecutorTest, FailingCommandInTheMiddle) {
    auto p = MakePipeline({{"false"}, {"true"}});
    EXPECT_EQ(executor.RunPipeline(p), 0);
}

TEST_F(ExecutorTest, MassiveOutputDoesNotDeadlock) {
    auto p = MakePipeline({{"yes", "test"}, {"head", "-n", "100"}});
    EXPECT_EQ(executor.RunPipeline(p), 0);
}

TEST_F(ExecutorTest, DirectoryExecutionAttemptReturns126) {
    auto p = MakePipeline({{"/"}});
    EXPECT_EQ(executor.RunPipeline(p), ExitCode::PERMISSION_DENIED);
}

TEST_F(ExecutorTest, KilledBySignalReturns128PlusSignal) {
    auto p = MakePipeline({{"sh", "-c", "kill -9 $$"}});
    EXPECT_EQ(executor.RunPipeline(p), 137);
}

TEST_F(ExecutorTest, MassiveArgumentsAllocation) {
    std::vector<std::string> args = {"echo"};

    for (int i = 0; i < 10000; ++i) {
        args.push_back("stress_arg");
    }
    auto p = MakePipeline({args});

    EXPECT_EQ(executor.RunPipeline(p), 0);
}

TEST_F(ExecutorTest, FileDescriptorLeakStressTest_100_Pipes) {
    std::vector<std::vector<std::string>> commands;
    commands.push_back({"echo", "stress_test"});

    for (int i = 0; i < 98; ++i) {
        commands.push_back({"cat"});
    }
    commands.push_back({"grep", "stress_test"});

    auto p = MakePipeline(commands);
    EXPECT_EQ(executor.RunPipeline(p), 0);
}

TEST_F(ExecutorTest, EmptyCommandInTheMiddleOfPipeline) {
    Pipeline p;
    Command cmd1, cmd2, cmd3;
    cmd1.args = {"echo", "hi"};
    cmd2.args = {};
    cmd3.args = {"cat"};

    p.commands.push_back(cmd1);
    p.commands.push_back(cmd2);
    p.commands.push_back(cmd3);

    EXPECT_NO_FATAL_FAILURE({
        int status = executor.RunPipeline(p);
        EXPECT_EQ(status, 0);
    });
}

TEST_F(ExecutorTest, MachineGun_1000_CommandsSpeedTest) {
    for (int i = 0; i < 1000; ++i) {
        auto p = MakePipeline({{"true"}});
        EXPECT_EQ(executor.RunPipeline(p), 0);
    }
}

TEST_F(ExecutorTest, IdioticInput_MaxArgsLimit) {
    std::vector<std::string> args = {"echo"};
    for (int i = 0; i < 50000; ++i) {
        args.push_back("NASTY_TEST_ARGUMENT");
    }
    auto p = MakePipeline({args});

    EXPECT_NO_FATAL_FAILURE({ executor.RunPipeline(p); });
}