#include <sstream>

#include "doctest.h"
#include "tokenizer/tokenizer.hpp"
#include "utils/yash_error.hpp"

TEST_CASE("SimpleCase") {
    std::stringstream ss{R"(ls | grep "somthing new")"};
    Tokenizer tokenizer{&ss};

    CHECK_FALSE(tokenizer.IsEnd());
    CHECK(tokenizer.GetToken() == Token{WordToken("ls")});

    tokenizer.Next();
    CHECK_FALSE(tokenizer.IsEnd());
    CHECK(tokenizer.GetToken() == Token{PipeToken()});

    tokenizer.Next();
    CHECK_FALSE(tokenizer.IsEnd());
    CHECK(tokenizer.GetToken() == Token{WordToken("grep")});

    tokenizer.Next();
    CHECK_FALSE(tokenizer.IsEnd());
    CHECK(tokenizer.GetToken() == Token{WordToken("somthing new")});

    tokenizer.Next();
    CHECK(tokenizer.IsEnd());
}

TEST_CASE("IsStreaming") {
    std::stringstream ss;
    ss << "ls ";
    Tokenizer tokenizer{&ss};
    CHECK(tokenizer.GetToken() == Token{WordToken("ls")});

    ss << "| grep ";
    tokenizer.Next();
    CHECK(tokenizer.GetToken() == Token{PipeToken{}});

    tokenizer.Next();
    CHECK(tokenizer.GetToken() == Token{WordToken("grep")});

    tokenizer.Next();
    CHECK(tokenizer.IsEnd());
}

TEST_CASE("Redirections") {
    std::stringstream ss{R"(cat < in.txt > out.txt >> append.txt)"};
    Tokenizer tokenizer{&ss};

    CHECK(tokenizer.GetToken() == Token{WordToken("cat")});

    tokenizer.Next();
    CHECK(tokenizer.GetToken() == Token{RedirectToken::REDIRECT_IN});

    tokenizer.Next();
    CHECK(tokenizer.GetToken() == Token{WordToken("in.txt")});

    tokenizer.Next();
    CHECK(tokenizer.GetToken() == Token{RedirectToken::REDIRECT_OUT});

    tokenizer.Next();
    CHECK(tokenizer.GetToken() == Token{WordToken("out.txt")});

    tokenizer.Next();
    CHECK(tokenizer.GetToken() == Token{RedirectToken::REDIRECT_APPEND});

    tokenizer.Next();
    CHECK(tokenizer.GetToken() == Token{WordToken("append.txt")});

    tokenizer.Next();
    CHECK(tokenizer.IsEnd());
}

TEST_CASE("GetTokenIsNotMoving") {
    std::stringstream ss{"ls grep"};
    Tokenizer tokenizer{&ss};

    CHECK(tokenizer.GetToken() == Token{WordToken("ls")});
    CHECK(tokenizer.GetToken() == Token{WordToken("ls")});

    tokenizer.Next();
    CHECK(tokenizer.GetToken() == Token{WordToken("grep")});
    CHECK(tokenizer.GetToken() == Token{WordToken("grep")});

    tokenizer.Next();
    CHECK(tokenizer.IsEnd());
}

TEST_CASE("SpacesAreHandled") {
    std::stringstream ss{"    "};
    Tokenizer tokenizer{&ss};
    CHECK(tokenizer.IsEnd());

    std::stringstream ss2{"  ls   |  grep  "};
    Tokenizer t2{&ss2};

    CHECK(t2.GetToken() == Token{WordToken("ls")});

    t2.Next();
    CHECK(t2.GetToken() == Token{PipeToken()});

    t2.Next();
    CHECK(t2.GetToken() == Token{WordToken("grep")});

    tokenizer.Next();
    CHECK(tokenizer.IsEnd());
}

TEST_CASE("EmptyString") {
    std::stringstream ss;
    Tokenizer tokenizer{&ss};

    CHECK(tokenizer.IsEnd());
}

TEST_CASE("NoSpaceNoDouble") {
    std::stringstream ss{"ls>trash.json"};
    Tokenizer tokenizer{&ss};
    CHECK_FALSE(tokenizer.IsEnd());

    CHECK(tokenizer.GetToken() == Token{WordToken("ls")});
    CHECK(tokenizer.GetToken() == Token{WordToken("ls")});
    CHECK(tokenizer.GetToken() == Token{WordToken("ls")});

    tokenizer.Next();
    CHECK(tokenizer.GetToken() == Token{RedirectToken::REDIRECT_OUT});

    tokenizer.Next();
    CHECK(tokenizer.GetToken() == Token{WordToken{"trash.json"}});

    tokenizer.Next();
    CHECK(tokenizer.IsEnd());
}

TEST_CASE("InsideQuotesNoSplit") {
    std::stringstream ss{R"(echo "hello | grep")"};
    Tokenizer tokenizer{&ss};
    CHECK_FALSE(tokenizer.IsEnd());

    CHECK(tokenizer.GetToken() == Token{WordToken("echo")});
    CHECK(tokenizer.GetToken() == Token{WordToken("echo")});
    CHECK(tokenizer.GetToken() == Token{WordToken("echo")});

    CHECK_FALSE(tokenizer.IsEnd());

    tokenizer.Next();
    CHECK(tokenizer.GetToken() == Token{WordToken{"hello | grep"}});
    CHECK(tokenizer.GetToken() == Token{WordToken{"hello | grep"}});
    CHECK(tokenizer.GetToken() == Token{WordToken{"hello | grep"}});

    tokenizer.Next();
    CHECK(tokenizer.IsEnd());
    CHECK(tokenizer.IsEnd());
    CHECK(tokenizer.GetToken() == Token{WordToken{"hello | grep"}});
    CHECK(tokenizer.IsEnd());
}

TEST_CASE("EmptyQuotes") {
    std::stringstream ss{R"(echo "")"};
    Tokenizer t{&ss};

    CHECK(t.GetToken() == Token{WordToken("echo")});

    t.Next();
    CHECK(t.GetToken() == Token{WordToken("")});

    t.Next();
    CHECK(t.IsEnd());
}

TEST_CASE("MixedQuotesSingleWord") {
    std::stringstream ss{R"(echo "hello"world'!')"};
    Tokenizer t{&ss};

    CHECK(t.GetToken() == Token{WordToken("echo")});

    t.Next();
    CHECK(t.GetToken() == Token{WordToken("helloworld!")});
}

TEST_CASE("UnclosedDoubleQuoteThrows") {
    std::stringstream ss{R"(echo "this is unclosed)"};
    Tokenizer t{&ss};

    CHECK(t.GetToken() == Token{WordToken("echo")});

    CHECK_THROWS_AS(t.Next(), YashSyntaxError);
}

TEST_CASE("UnclosedSingleQuoteThrows") {
    std::stringstream ss{R"(echo 'this is unclosed)"};
    Tokenizer t{&ss};

    CHECK(t.GetToken() == Token{WordToken("echo")});

    CHECK_THROWS_AS(t.Next(), YashSyntaxError);
}