#include "executor/executor.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <sys/stat.h>

std::string ReadFileContents(const std::string& path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);

    if (!in) {
        return "";
    }

    std::string contents;
    in.seekg(0, std::ios::end);
    contents.resize(in.tellg());
    in.seekg(0, std::ios::beg);
    in.read(&contents[0], contents.size());
    return contents;
}

// ============================================================================
// REDIRECTION TESTS
// ============================================================================

class RedirectionTest : public ::testing::Test {
protected:
    Executor executor;
    const std::string test_dir = "/tmp/yash_redir_tests";
    const std::string out_file = "/tmp/yash_redir_tests/out.txt";
    const std::string in_file = "/tmp/yash_redir_tests/in.txt";
    const std::string no_perm_file = "/tmp/yash_redir_tests/no_perm.txt";

    void SetUp() override {
        std::filesystem::create_directory(test_dir);

        std::ofstream in(in_file);
        in << "hello_from_input\n";
        in.close();

        std::ofstream np(no_perm_file);
        np << "secret";
        np.close();
        chmod(no_perm_file.c_str(), S_IRUSR);
    }

    void TearDown() override {
        std::filesystem::remove_all(test_dir);
    }

    Pipeline MakeRedirPipeline(
        std::vector<std::string> args,
        std::string in = "",
        std::string out = "",
        bool append = false
    ) {
        Command cmd;
        cmd.args = std::move(args);
        cmd.redirect_in = std::move(in);
        cmd.redirect_out = std::move(out);
        cmd.append_out = append;

        Pipeline p;
        p.commands.push_back(std::move(cmd));
        return p;
    }
};

TEST_F(RedirectionTest, OutputRedirectionTruncatesFile) {
    std::ofstream f(out_file);
    f << "OLD_TRASH_DATA_THAT_SHOULD_BE_DELETED";
    f.close();

    auto p = MakeRedirPipeline({"echo", "-n", "new_data"}, "", out_file, false);
    EXPECT_EQ(executor.RunPipeline(p), 0);

    EXPECT_EQ(ReadFileContents(out_file), "new_data");
}

TEST_F(RedirectionTest, OutputRedirectionAppendsToFile) {
    std::ofstream f(out_file);
    f << "line1\n";
    f.close();

    auto p = MakeRedirPipeline({"echo", "-n", "line2"}, "", out_file, true);
    EXPECT_EQ(executor.RunPipeline(p), 0);

    EXPECT_EQ(ReadFileContents(out_file), "line1\nline2");
}

TEST_F(RedirectionTest, InputRedirectionReadsFromFile) {
    auto p = MakeRedirPipeline({"cat"}, in_file, out_file, false);
    EXPECT_EQ(executor.RunPipeline(p), 0);

    EXPECT_EQ(ReadFileContents(out_file), "hello_from_input\n");
}

TEST_F(RedirectionTest, CombinedInputAndOutputRedirection) {
    auto p = MakeRedirPipeline({"tr", "a-z", "A-Z"}, in_file, out_file, false);
    EXPECT_EQ(executor.RunPipeline(p), 0);

    EXPECT_EQ(ReadFileContents(out_file), "HELLO_FROM_INPUT\n");
}

TEST_F(RedirectionTest, InputFromNonExistentFileFails) {
    auto p = MakeRedirPipeline({"cat"}, "/tmp/yash_redir_tests/GHOST_FILE.txt", "", false);

    int status = executor.RunPipeline(p);
    EXPECT_NE(status, 0);
}

TEST_F(RedirectionTest, OutputToReadOnlyFileFails) {
    auto p = MakeRedirPipeline({"echo", "hacker"}, "", no_perm_file, false);

    int status = executor.RunPipeline(p);
    EXPECT_NE(status, 0);

    EXPECT_EQ(ReadFileContents(no_perm_file), "secret");
}

TEST_F(RedirectionTest, OutputToDirectoryFails) {
    auto p = MakeRedirPipeline({"echo", "test"}, "", test_dir, false);

    int status = executor.RunPipeline(p);
    EXPECT_NE(status, 0);
}