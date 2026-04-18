#pragma once

#include <cstdint>
#include <variant>
#include <istream>
#include <string>

struct WordToken {
    std::string value;

    WordToken() = default;
    WordToken(std::string val) : value(std::move(val)) {
    }

    bool operator==(const std::string& other) const {
        return this->value == other;
    }

    bool operator==(const WordToken& other) const {
        return this->value == other.value;
    };
};

enum struct RedirectToken : std::uint8_t { REDIRECT_OUT, REDIRECT_IN, REDIRECT_APPEND, HERE_DOC };

struct PipeToken {
    bool operator==(const PipeToken&) const {
        return true;
    };
};

using Token = std::variant<WordToken, RedirectToken, PipeToken>;

class Tokenizer {
public:
    Tokenizer(std::istream* in);

    bool IsEnd();

    void Next();

    Token GetToken();

private:
    std::istream* original_data_;
    bool is_end_ = false;

    Token current_token_;
};