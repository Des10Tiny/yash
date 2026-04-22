#pragma once

#include <optional>
#include <string>
#include <vector>
#include "tokenizer/tokenizer.hpp"

struct Command {
    std::string value;
    std::vector<std::string> args;
    std::string redirect_in;
    std::string redirect_out;
    bool append_out = false;
};

struct Pipeline {
    std::vector<Command> commands;
};

class Parser {
public:
    Parser(Tokenizer& tokenizer) : tokenizer_(tokenizer) {
    }

    std::optional<Pipeline> ParseAll();

private:
    Tokenizer& tokenizer_;
};