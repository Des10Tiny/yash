#include "gtest/gtest.h"
#include "tokenizer/tokenizer.hpp"

TEST(TokenizerTest, SimpleCase) {
    std::stringstream ss{R"(ls | grep "somthing new")"};
    Tokenizer tokenizer{&ss};

    EXPECT_FALSE(tokenizer.IsEnd());
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("ls")});

    tokenizer.Next();
    EXPECT_FALSE(tokenizer.IsEnd());
    EXPECT_EQ(tokenizer.GetToken(), Token{PipeToken()});

    tokenizer.Next();
    EXPECT_FALSE(tokenizer.IsEnd());
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("grep")});

    tokenizer.Next();
    EXPECT_FALSE(tokenizer.IsEnd());
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("somthing new")});

    tokenizer.Next();
    EXPECT_TRUE(tokenizer.IsEnd());
}

TEST(TokenizerTest, IsStreaming) {
    std::stringstream ss;
    ss << "ls ";
    Tokenizer tokenizer{&ss};
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("ls")});

    ss << "| grep ";
    tokenizer.Next();
    EXPECT_EQ(tokenizer.GetToken(), Token{PipeToken{}});

    tokenizer.Next();
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("grep")});

    tokenizer.Next();
    EXPECT_TRUE(tokenizer.IsEnd());
}

TEST(TokenizerTest, Redirections) {
    std::stringstream ss{R"(cat < in.txt > out.txt >> append.txt)"};
    Tokenizer tokenizer{&ss};

    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("cat")});

    tokenizer.Next();
    EXPECT_EQ(tokenizer.GetToken(), Token{RedirectToken::REDIRECT_IN});

    tokenizer.Next();
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("in.txt")});

    tokenizer.Next();
    EXPECT_EQ(tokenizer.GetToken(), Token{RedirectToken::REDIRECT_OUT});

    tokenizer.Next();
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("out.txt")});

    tokenizer.Next();
    EXPECT_EQ(tokenizer.GetToken(), Token{RedirectToken::REDIRECT_APPEND});

    tokenizer.Next();
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("append.txt")});

    tokenizer.Next();
    EXPECT_TRUE(tokenizer.IsEnd());
}

TEST(TokenizerTest, GetTokenIsNotMoving) {
    std::stringstream ss{"ls grep"};
    Tokenizer tokenizer{&ss};

    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("ls")});
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("ls")});

    tokenizer.Next();
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("grep")});
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("grep")});

    tokenizer.Next();
    EXPECT_TRUE(tokenizer.IsEnd());
}

TEST(TokenizerTest, SpacesAreHandled) {
    std::stringstream ss{"    "};
    Tokenizer tokenizer{&ss};
    EXPECT_TRUE(tokenizer.IsEnd());

    std::stringstream ss2{"  ls   |  grep  "};
    Tokenizer t2{&ss2};

    EXPECT_EQ(t2.GetToken(), Token{WordToken("ls")});

    t2.Next();
    EXPECT_EQ(t2.GetToken(), Token{PipeToken()});

    t2.Next();
    EXPECT_EQ(t2.GetToken(), Token{WordToken("grep")});

    tokenizer.Next();
    EXPECT_TRUE(tokenizer.IsEnd());
}

TEST(TokenizerTest, EmptyString) {
    std::stringstream ss;
    Tokenizer tokenizer{&ss};

    EXPECT_TRUE(tokenizer.IsEnd());
}

TEST(TokenizerTest, NoSpaceNoDouble) {
    std::stringstream ss{"ls>trash.json"};
    Tokenizer tokenizer{&ss};
    EXPECT_FALSE(tokenizer.IsEnd());

    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("ls")});
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("ls")});
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("ls")});

    tokenizer.Next();
    EXPECT_EQ(tokenizer.GetToken(), Token{RedirectToken::REDIRECT_OUT});

    tokenizer.Next();
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken{"trash.json"}});

    tokenizer.Next();
    EXPECT_TRUE(tokenizer.IsEnd());
}

TEST(TokenizerTest, InsideQuotesNoSplit) {
    std::stringstream ss{R"(echo "hello | grep")"};
    Tokenizer tokenizer{&ss};
    EXPECT_FALSE(tokenizer.IsEnd());

    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("echo")});
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("echo")});
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken("echo")});

    EXPECT_FALSE(tokenizer.IsEnd());

    tokenizer.Next();
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken{"hello | grep"}});
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken{"hello | grep"}});
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken{"hello | grep"}});

    tokenizer.Next();
    EXPECT_TRUE(tokenizer.IsEnd());
    EXPECT_TRUE(tokenizer.IsEnd());
    EXPECT_EQ(tokenizer.GetToken(), Token{WordToken{"hello | grep"}});
    EXPECT_TRUE(tokenizer.IsEnd());
}

TEST(TokenizerTest, EmptyQuotes) {
    std::stringstream ss{R"(echo "")"};
    Tokenizer t{&ss};

    EXPECT_EQ(t.GetToken(), Token{WordToken("echo")});

    t.Next();
    EXPECT_EQ(t.GetToken(), Token{WordToken("")});

    t.Next();
    EXPECT_TRUE(t.IsEnd());
}

TEST(TokenizerTest, MixedQuotesSingleWord) {
    std::stringstream ss{R"(echo "hello"world'!')"};
    Tokenizer t{&ss};

    EXPECT_EQ(t.GetToken(), Token{WordToken("echo")});

    t.Next();
    EXPECT_EQ(t.GetToken(), Token{WordToken("helloworld!")});
}

TEST(TokenizerTest, UnclosedDoubleQuoteThrows) {
    std::stringstream ss{R"(echo "this is unclosed)"};
    Tokenizer t{&ss};

    EXPECT_EQ(t.GetToken(), Token{WordToken("echo")});

    EXPECT_THROW(t.Next(), std::runtime_error);
}

TEST(TokenizerTest, UnclosedSingleQuoteThrows) {
    std::stringstream ss{R"(echo 'this is unclosed)"};
    Tokenizer t{&ss};

    EXPECT_EQ(t.GetToken(), Token{WordToken("echo")});

    EXPECT_THROW(t.Next(), std::runtime_error);
}