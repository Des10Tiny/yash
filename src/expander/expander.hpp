#pragma once

#include "tokenizer/itokenizer.hpp"

#include <deque>
#include <unordered_map>

class Expander : public ITokenizer {
public:
    Expander(
        ITokenizer& base_tokenizer, const std::unordered_map<std::string, std::string>& aliases
    )
        : base_tokenizer_(base_tokenizer)
        , aliases_(aliases) {
            current_token_ = base_tokenizer_.GetToken();
        };

    ~Expander() override = default;

    [[nodiscard]] bool IsEnd() const override;

    void Next() override;

    Token GetToken() override;

private:
    ITokenizer& base_tokenizer_;
    std::unordered_map<std::string, std::string> aliases_;

    std::deque<Token> buffer_;
    bool is_command_position_ = true;

    Token current_token_;
    bool is_end_ = false;
};