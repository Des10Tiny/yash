#include <gtest/gtest.h>
#include <sstream>
#include <stdexcept>

#include "parser/parser.hpp"
#include "utils/yash_error.hpp"

std::optional<Pipeline> ParseString(const std::string& input) {
    std::stringstream ss{input};
    Tokenizer tokenizer{&ss};
    Parser parser{tokenizer};
    return parser.ParsePipeline();
}

TEST(ParserTest, EmptyInputReturnsNullopt) {
    EXPECT_FALSE(ParseString("").has_value());
    EXPECT_FALSE(ParseString("    \t   ").has_value());
}

TEST(ParserTest, SingleCommandNoArgs) {
    auto result = ParseString("ls");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->commands.size(), 1);
    EXPECT_EQ(result->commands[0].args.size(), 1);
    EXPECT_EQ(result->commands[0].args[0], "ls");
}

TEST(ParserTest, SingleCommandWithArgs) {
    auto result = ParseString("grep -v -i \"test string\"");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->commands[0].args.size(), 4);
    EXPECT_EQ(result->commands[0].args[0], "grep");
    EXPECT_EQ(result->commands[0].args[1], "-v");
    EXPECT_EQ(result->commands[0].args[2], "-i");
    EXPECT_EQ(result->commands[0].args[3], "test string");
}

TEST(ParserTest, RedirectOut) {
    auto result = ParseString("echo hello > output.txt");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->commands[0].args[0], "echo");
    EXPECT_EQ(result->commands[0].args[1], "hello");
    EXPECT_EQ(result->commands[0].redirect_out, "output.txt");
    EXPECT_FALSE(result->commands[0].append_out);
}

TEST(ParserTest, RedirectAppend) {
    auto result = ParseString("cat log.txt >> all_logs.txt");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->commands[0].redirect_out, "all_logs.txt");
    EXPECT_TRUE(result->commands[0].append_out);
}

TEST(ParserTest, RedirectInAndOut) {
    auto result = ParseString("sort < unsorted.txt > sorted.txt");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->commands[0].redirect_in, "unsorted.txt");
    EXPECT_EQ(result->commands[0].redirect_out, "sorted.txt");
    EXPECT_FALSE(result->commands[0].append_out);
}

TEST(ParserTest, RedirectWithoutCommandIsStillValid) {

    auto result = ParseString("> file.txt");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->commands[0].args.empty());
    EXPECT_EQ(result->commands[0].redirect_out, "file.txt");
}

TEST(ParserTest, TwoCommandsPipe) {
    auto result = ParseString("ls -la | grep txt");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->commands.size(), 2);

    EXPECT_EQ(result->commands[0].args[0], "ls");
    EXPECT_EQ(result->commands[0].args[1], "-la");

    EXPECT_EQ(result->commands[1].args[0], "grep");
    EXPECT_EQ(result->commands[1].args[1], "txt");
}

TEST(ParserTest, ThreeCommandsPipe) {
    auto result = ParseString("cat file | grep word | wc -l");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->commands.size(), 3);
    EXPECT_EQ(result->commands[0].args[0], "cat");
    EXPECT_EQ(result->commands[1].args[0], "grep");
    EXPECT_EQ(result->commands[2].args[0], "wc");
}

TEST(ParserTest, PipesWithRedirects) {
    auto result = ParseString("cat < input.txt | grep error > errors.log");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->commands.size(), 2);

    EXPECT_EQ(result->commands[0].redirect_in, "input.txt");
    EXPECT_TRUE(result->commands[0].redirect_out.empty());

    EXPECT_EQ(result->commands[1].args[0], "grep");
    EXPECT_EQ(result->commands[1].redirect_out, "errors.log");
}

TEST(ParserTest, ErrorMissingFileAfterRedirectOut) {
    EXPECT_THROW(ParseString("echo hello >"), YashSyntaxError);
    EXPECT_THROW(ParseString("echo hello >    "), YashSyntaxError);
}

TEST(ParserTest, ErrorWrongTokenAfterRedirect) {
    EXPECT_THROW(ParseString("ls > | grep"), YashSyntaxError);
    EXPECT_THROW(ParseString("ls > > log.txt"), YashSyntaxError);
    EXPECT_THROW(ParseString("ls < <"), YashSyntaxError);
}

TEST(ParserTest, ErrorDanglingPipe) {
    EXPECT_THROW(ParseString("ls |"), YashSyntaxError);
    EXPECT_THROW(ParseString("ls |    "), YashSyntaxError);
}

TEST(ParserTest, ErrorDoublePipe) {
    // Untill i make ||
    EXPECT_THROW(ParseString("ls || grep"), YashSyntaxError);
    EXPECT_THROW(ParseString("ls | | grep"), YashSyntaxError);
}

TEST(ParserTest, ErrorPipeAtStart) {
    EXPECT_THROW(ParseString("| ls"), YashSyntaxError);
}

TEST(ParserTest, RedirectInTheMiddleOfArgs) {
    auto result = ParseString("grep < input.txt -v \"pattern\"");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->commands[0].args.size(), 3);
    EXPECT_EQ(result->commands[0].args[0], "grep");
    EXPECT_EQ(result->commands[0].args[1], "-v");
    EXPECT_EQ(result->commands[0].args[2], "pattern");
    EXPECT_EQ(result->commands[0].redirect_in, "input.txt");
}

TEST(ParserTest, MultipleRedirectsOverride) {
    auto result = ParseString("echo test > 1.txt > 2.txt");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->commands[0].redirect_out, "2.txt");
}

TEST(ParserTest, RedirectBeforeCommand) {
    auto result = ParseString("< input.txt cat -n");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->commands[0].args.size(), 2);
    EXPECT_EQ(result->commands[0].args[0], "cat");
    EXPECT_EQ(result->commands[0].args[1], "-n");
    EXPECT_EQ(result->commands[0].redirect_in, "input.txt");
}

TEST(ParserTest, SpacedMessWithRedirects) {
    auto result = ParseString("   ls   -la  >   out.txt   |   grep  txt   < in.txt  ");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->commands.size(), 2);

    EXPECT_EQ(result->commands[0].args[0], "ls");
    EXPECT_EQ(result->commands[0].args[1], "-la");
    EXPECT_EQ(result->commands[0].redirect_out, "out.txt");

    EXPECT_EQ(result->commands[1].args[0], "grep");
    EXPECT_EQ(result->commands[1].args[1], "txt");
    EXPECT_EQ(result->commands[1].redirect_in, "in.txt");
}

TEST(ParserTest, OnlyRedirectsNoCommand) {
    auto result = ParseString("< input.txt > output.txt");
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->commands[0].args.empty());
    EXPECT_EQ(result->commands[0].redirect_in, "input.txt");
    EXPECT_EQ(result->commands[0].redirect_out, "output.txt");
}

TEST(ParserTest, UltimateRedirectOverride) {
    auto result = ParseString("cat < 1.txt < 2.txt > 3.txt > 4.txt");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->commands[0].args[0], "cat");
    EXPECT_EQ(result->commands[0].redirect_in, "2.txt");
    EXPECT_EQ(result->commands[0].redirect_out, "4.txt");
    EXPECT_FALSE(result->commands[0].append_out);
}