#include <sstream>

#include "doctest.h"
#include "parser/parser.hpp"
#include "tokenizer/tokenizer.hpp"
#include "utils/yash_error.hpp"

std::optional<Pipeline> ParseString(const std::string& input) {
    std::stringstream ss{input};
    Tokenizer tokenizer{&ss};
    Parser parser{tokenizer};
    return parser.ParsePipeline();
}

TEST_CASE("EmptyInputReturnsNullopt") {
    CHECK_FALSE(ParseString("").has_value());
    CHECK_FALSE(ParseString("    \t   ").has_value());
}

TEST_CASE("SingleCommandNoArgs") {
    auto result = ParseString("ls");
    REQUIRE(result.has_value());
    CHECK(result->commands.size() == 1);
    CHECK(result->commands[0].args.size() == 1);
    CHECK(result->commands[0].args[0] == "ls");
}

TEST_CASE("SingleCommandWithArgs") {
    auto result = ParseString("grep -v -i \"test string\"");
    REQUIRE(result.has_value());
    CHECK(result->commands[0].args.size() == 4);
    CHECK(result->commands[0].args[0] == "grep");
    CHECK(result->commands[0].args[1] == "-v");
    CHECK(result->commands[0].args[2] == "-i");
    CHECK(result->commands[0].args[3] == "test string");
}

TEST_CASE("RedirectOut") {
    auto result = ParseString("echo hello > output.txt");
    REQUIRE(result.has_value());
    CHECK(result->commands[0].args[0] == "echo");
    CHECK(result->commands[0].args[1] == "hello");
    CHECK(result->commands[0].redirect_out == "output.txt");
    CHECK_FALSE(result->commands[0].append_out);
}

TEST_CASE("RedirectAppend") {
    auto result = ParseString("cat log.txt >> all_logs.txt");
    REQUIRE(result.has_value());
    CHECK(result->commands[0].redirect_out == "all_logs.txt");
    CHECK(result->commands[0].append_out);
}

TEST_CASE("RedirectInAndOut") {
    auto result = ParseString("sort < unsorted.txt > sorted.txt");
    REQUIRE(result.has_value());
    CHECK(result->commands[0].redirect_in == "unsorted.txt");
    CHECK(result->commands[0].redirect_out == "sorted.txt");
    CHECK_FALSE(result->commands[0].append_out);
}

TEST_CASE("RedirectWithoutCommandIsStillValid") {
    auto result = ParseString("> file.txt");
    REQUIRE(result.has_value());
    CHECK(result->commands[0].args.empty());
    CHECK(result->commands[0].redirect_out == "file.txt");
}

TEST_CASE("TwoCommandsPipe") {
    auto result = ParseString("ls -la | grep txt");
    REQUIRE(result.has_value());
    REQUIRE(result->commands.size() == 2);

    CHECK(result->commands[0].args[0] == "ls");
    CHECK(result->commands[0].args[1] == "-la");

    CHECK(result->commands[1].args[0] == "grep");
    CHECK(result->commands[1].args[1] == "txt");
}

TEST_CASE("ThreeCommandsPipe") {
    auto result = ParseString("cat file | grep word | wc -l");
    REQUIRE(result.has_value());
    REQUIRE(result->commands.size() == 3);
    CHECK(result->commands[0].args[0] == "cat");
    CHECK(result->commands[1].args[0] == "grep");
    CHECK(result->commands[2].args[0] == "wc");
}

TEST_CASE("PipesWithRedirects") {
    auto result = ParseString("cat < input.txt | grep error > errors.log");
    REQUIRE(result.has_value());
    REQUIRE(result->commands.size() == 2);

    CHECK(result->commands[0].redirect_in == "input.txt");
    CHECK(result->commands[0].redirect_out.empty());

    CHECK(result->commands[1].args[0] == "grep");
    CHECK(result->commands[1].redirect_out == "errors.log");
}

TEST_CASE("ErrorMissingFileAfterRedirectOut") {
    CHECK_THROWS_AS(ParseString("echo hello >"), YashSyntaxError);
    CHECK_THROWS_AS(ParseString("echo hello >    "), YashSyntaxError);
}

TEST_CASE("ErrorWrongTokenAfterRedirect") {
    CHECK_THROWS_AS(ParseString("ls > | grep"), YashSyntaxError);
    CHECK_THROWS_AS(ParseString("ls > > log.txt"), YashSyntaxError);
    CHECK_THROWS_AS(ParseString("ls < <"), YashSyntaxError);
}

TEST_CASE("ErrorDanglingPipe") {
    CHECK_THROWS_AS(ParseString("ls |"), YashSyntaxError);
    CHECK_THROWS_AS(ParseString("ls |    "), YashSyntaxError);
}

TEST_CASE("ErrorDoublePipe") {
    // Untill i make ||
    CHECK_THROWS_AS(ParseString("ls || grep"), YashSyntaxError);
    CHECK_THROWS_AS(ParseString("ls | | grep"), YashSyntaxError);
}

TEST_CASE("ErrorPipeAtStart") {
    CHECK_THROWS_AS(ParseString("| ls"), YashSyntaxError);
}

TEST_CASE("RedirectInTheMiddleOfArgs") {
    auto result = ParseString("grep < input.txt -v \"pattern\"");
    REQUIRE(result.has_value());
    CHECK(result->commands[0].args.size() == 3);
    CHECK(result->commands[0].args[0] == "grep");
    CHECK(result->commands[0].args[1] == "-v");
    CHECK(result->commands[0].args[2] == "pattern");
    CHECK(result->commands[0].redirect_in == "input.txt");
}

TEST_CASE("MultipleRedirectsOverride") {
    auto result = ParseString("echo test > 1.txt > 2.txt");
    REQUIRE(result.has_value());
    CHECK(result->commands[0].redirect_out == "2.txt");
}

TEST_CASE("RedirectBeforeCommand") {
    auto result = ParseString("< input.txt cat -n");
    REQUIRE(result.has_value());
    CHECK(result->commands[0].args.size() == 2);
    CHECK(result->commands[0].args[0] == "cat");
    CHECK(result->commands[0].args[1] == "-n");
    CHECK(result->commands[0].redirect_in == "input.txt");
}

TEST_CASE("SpacedMessWithRedirects") {
    auto result = ParseString("   ls   -la  >   out.txt   |   grep  txt   < in.txt  ");
    REQUIRE(result.has_value());
    REQUIRE(result->commands.size() == 2);

    CHECK(result->commands[0].args[0] == "ls");
    CHECK(result->commands[0].args[1] == "-la");
    CHECK(result->commands[0].redirect_out == "out.txt");

    CHECK(result->commands[1].args[0] == "grep");
    CHECK(result->commands[1].args[1] == "txt");
    CHECK(result->commands[1].redirect_in == "in.txt");
}

TEST_CASE("OnlyRedirectsNoCommand") {
    auto result = ParseString("< input.txt > output.txt");
    REQUIRE(result.has_value());
    CHECK(result->commands[0].args.empty());
    CHECK(result->commands[0].redirect_in == "input.txt");
    CHECK(result->commands[0].redirect_out == "output.txt");
}

TEST_CASE("UltimateRedirectOverride") {
    auto result = ParseString("cat < 1.txt < 2.txt > 3.txt > 4.txt");
    REQUIRE(result.has_value());
    CHECK(result->commands[0].args[0] == "cat");
    CHECK(result->commands[0].redirect_in == "2.txt");
    CHECK(result->commands[0].redirect_out == "4.txt");
    CHECK_FALSE(result->commands[0].append_out);
}