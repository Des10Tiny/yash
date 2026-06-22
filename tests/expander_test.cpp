#include "doctest.h"
#include "expander/expander.hpp"
#include "tokenizer/tokenizer.hpp"

#include <sstream>
#include <string>
#include <vector>

std::string TokenToString(const Token& token) {
    if (const auto* word = std::get_if<WordToken>(&token)) {
        return word->value;
    }

    if (std::holds_alternative<PipeToken>(token)) {
        return "|";
    }

    if (const auto* redir = std::get_if<RedirectToken>(&token)) {
        switch (*redir) {
        case RedirectToken::REDIRECT_OUT:
            return ">";
        case RedirectToken::REDIRECT_IN:
            return "<";
        case RedirectToken::REDIRECT_APPEND:
            return ">>";
        case RedirectToken::HERE_DOC:
            return "<<";
        }
    }

    return "UNKNOWN";
}

std::vector<std::string>
RunExpander(const std::string& input, const std::unordered_map<std::string, std::string>& aliases) {
    std::istringstream ss(input);
    Tokenizer base_tokenizer(&ss);
    Expander expander(base_tokenizer, aliases);

    std::vector<std::string> result;

    while (!expander.IsEnd()) {
        result.push_back(TokenToString(expander.GetToken()));
        expander.Next();
    }

    return result;
}

TEST_CASE("Expander: No aliases defined") {
    std::unordered_map<std::string, std::string> aliases;

    auto tokens = RunExpander("echo hello | cat", aliases);

    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0] == "echo");
    CHECK(tokens[1] == "hello");
    CHECK(tokens[2] == "|");
    CHECK(tokens[3] == "cat");
}

TEST_CASE("Expander: Basic alias in command position") {
    std::unordered_map<std::string, std::string> aliases = {{"ll", "ls -l -a"}};

    auto tokens = RunExpander("ll", aliases);

    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0] == "ls");
    CHECK(tokens[1] == "-l");
    CHECK(tokens[2] == "-a");
}

TEST_CASE("Expander: Ignores alias in argument position") {
    std::unordered_map<std::string, std::string> aliases = {{"ll", "ls -la"}};

    auto tokens = RunExpander("echo ll", aliases);

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0] == "echo");
    CHECK(tokens[1] == "ll");
}

TEST_CASE("Expander: Expands alias after PipeToken") {
    std::unordered_map<std::string, std::string> aliases = {{"grep", "grep --color=auto"}};

    auto tokens = RunExpander("cat file.txt | grep error", aliases);

    REQUIRE(tokens.size() == 6);
    CHECK(tokens[0] == "cat");
    CHECK(tokens[1] == "file.txt");
    CHECK(tokens[2] == "|");
    CHECK(tokens[3] == "grep");
    CHECK(tokens[4] == "--color=auto");
    CHECK(tokens[5] == "error");
}

TEST_CASE("Expander: Same word as alias and argument") {
    std::unordered_map<std::string, std::string> aliases = {{"vim", "nvim"}};

    auto tokens = RunExpander("vim vim", aliases);

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0] == "nvim");
    CHECK(tokens[1] == "vim");
}

TEST_CASE("Expander: Multiple aliases in complex pipeline") {
    std::unordered_map<std::string, std::string> aliases = {{"ll", "ls -la"}, {"cat", "bat"}};

    auto tokens = RunExpander("ll | cat", aliases);

    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0] == "ls");
    CHECK(tokens[1] == "-la");
    CHECK(tokens[2] == "|");
    CHECK(tokens[3] == "bat");
}

TEST_CASE("Expander: Alias with quotes inside (Preserves tokenizer logic)") {
    std::unordered_map<std::string, std::string> aliases = {{"greet", "echo \"Hello, World!\""}};

    auto tokens = RunExpander("greet", aliases);

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0] == "echo");
    CHECK(tokens[1] == "Hello, World!");
}

TEST_CASE("Expander: Redirections do not trigger command position") {
    std::unordered_map<std::string, std::string> aliases = {{"ll", "ls -la"}};

    auto tokens = RunExpander("echo > out.txt ll", aliases);

    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0] == "echo");
    CHECK(tokens[1] == ">");
    CHECK(tokens[2] == "out.txt");
    CHECK(tokens[3] == "ll");
}

TEST_CASE("Expander: Redirects first") {
    std::unordered_map<std::string, std::string> aliases = {{"ll", "ls -lh"}};

    auto tokens = RunExpander("> output.txt ll", aliases);

    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0] == ">");
    CHECK(tokens[1] == "output.txt");
    CHECK(tokens[2] == "ls");
    CHECK(tokens[3] == "-lh");
}

TEST_CASE("Expander: Redirects DO NOT break command position") {
    std::unordered_map<std::string, std::string> aliases = {{"ll", "ls -la"}};

    auto tokens = RunExpander("> out.txt ll src", aliases);

    REQUIRE(tokens.size() == 5);
    CHECK(tokens[0] == ">");
    CHECK(tokens[1] == "out.txt");
    CHECK(tokens[2] == "ls");
    CHECK(tokens[3] == "-la");
    CHECK(tokens[4] == "src");
}

TEST_CASE("Expander: Alias containing a PipeToken") {
    std::unordered_map<std::string, std::string> aliases = {{"log", "cat log.txt | grep ERROR"}};

    auto tokens = RunExpander("log", aliases);

    REQUIRE(tokens.size() == 5);
    CHECK(tokens[0] == "cat");
    CHECK(tokens[1] == "log.txt");
    CHECK(tokens[2] == "|");
    CHECK(tokens[3] == "grep");
    CHECK(tokens[4] == "ERROR");
}

TEST_CASE("Expander: Recursive alias prevention (Single Pass)") {
    std::unordered_map<std::string, std::string> aliases = {{"ls", "ls --color"}};

    auto tokens = RunExpander("ls", aliases);

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0] == "ls");
    CHECK(tokens[1] == "--color");
}

TEST_CASE("Expander: Consecutive empty aliases") {
    std::unordered_map<std::string, std::string> aliases = {{"a", ""}, {"b", ""}};

    auto tokens = RunExpander("a b echo ok", aliases);

    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0] == "b");
    CHECK(tokens[1] == "echo");
    CHECK(tokens[2] == "ok");
}

TEST_CASE("Expander ROTTEN: Alias name matches redirect target") {
    std::unordered_map<std::string, std::string> aliases = {{"ll", "ls -la"}};

    auto tokens = RunExpander("> ll ll", aliases);

    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0] == ">");
    CHECK(tokens[1] == "ll");
    CHECK(tokens[2] == "ls");
    CHECK(tokens[3] == "-la");
}

TEST_CASE("Expander ROTTEN: Multiple redirects before command") {
    std::unordered_map<std::string, std::string> aliases = {{"grep", "grep -E"}};

    auto tokens = RunExpander("< input.txt > output.txt >> append.log grep pattern", aliases);

    REQUIRE(tokens.size() == 9);
    CHECK(tokens[6] == "grep");
    CHECK(tokens[7] == "-E");
    CHECK(tokens[8] == "pattern");
}

TEST_CASE("Expander ROTTEN: Empty alias right before command") {
    std::unordered_map<std::string, std::string> aliases = {{"null", ""}, {"ll", "ls -la"}};

    auto tokens = RunExpander("null > out.txt ll", aliases);

    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0] == ">");
    CHECK(tokens[1] == "out.txt");
    CHECK(tokens[2] == "ll");
}

TEST_CASE("Expander ROTTEN: Alias expanding to a redirect (Insane user)") {
    std::unordered_map<std::string, std::string> aliases = {{"logme", "> output.log"}};

    auto tokens = RunExpander("logme", aliases);

    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0] == ">");
    CHECK(tokens[1] == "output.log");
}