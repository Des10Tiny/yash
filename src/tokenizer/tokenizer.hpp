#pragma once
#include "itokenizer.hpp"

class Tokenizer : public ITokenizer {
public:
    Tokenizer(std::istream* in);

    [[nodiscard]] bool IsEnd() const override;

    void Next() override;

    Token GetToken() override;

private:
    std::istream* original_data_;
    bool is_end_ = false;

    Token current_token_;
};