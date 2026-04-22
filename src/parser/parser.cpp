#include "parser.hpp"

std::optional<Pipeline> Parser::ParseAll() {
    tokenizer_.Next();
    return std::nullopt;
}