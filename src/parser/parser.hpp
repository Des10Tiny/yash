#pragma once

#include "tokenizer/tokenizer.hpp"
#include <optional>
#include <string>
#include <vector>

struct Command {
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
    Parser(Tokenizer& tokenizer)
        : tokenizer_(tokenizer) {
    }

    std::optional<Pipeline> ParsePipeline();

private:
    std::optional<Command> ParseCommand();
    Tokenizer& tokenizer_;
};