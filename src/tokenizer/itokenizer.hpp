#pragma once

#include <cstdint>
#include <string>
#include <variant>

struct WordToken {
    std::string value;

    WordToken() = default;
    WordToken(std::string val)
        : value(std::move(val)) {
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

class ITokenizer {
public:
    virtual ~ITokenizer() = default;

    [[nodiscard]] virtual bool IsEnd() const = 0;

    virtual void Next() = 0;

    virtual Token GetToken() = 0;
};