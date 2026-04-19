#include "tokenizer.hpp"
#include <cctype>
#include <cstdio>
#include <stdexcept>
#include <string>

Tokenizer::Tokenizer(std::istream* in) : original_data_(in) {
    Next();
}

bool Tokenizer::IsEnd() {
    return is_end_;
}

Token Tokenizer::GetToken() {
    return current_token_;
}
std::string ExtractValue(std::istream* data, bool& is_end, char started_symbol) {
    std::string result_value;
    result_value += static_cast<unsigned char>(started_symbol);

    while (true) {
        int curr_char = data->get();

        if (curr_char == EOF) {
            is_end = true;
            break;
        }

        if (std::isspace(static_cast<unsigned char>(curr_char))) {
            break;
        }

        if (static_cast<unsigned char>(curr_char) <= 127) {
            result_value += curr_char;
            continue;
        }

        throw std::runtime_error("Find non ASCII symbol");
    }

    return result_value;
}

void Tokenizer::Next() {
    if (IsEnd()) {
        return;
    }

    while (true) {
        int current_char = original_data_->get();

        if (current_char == EOF) {
            is_end_ = true;
            return;
        }

        if (std::isspace(static_cast<unsigned char>(current_char))) {
            continue;
        }

        if (static_cast<char>(current_char) == '|') {
            current_token_ = PipeToken{};
            return;
        }

        if (static_cast<char>(current_char) == '>') {
            int next_char = original_data_->peek();

            if (next_char == '>') {
                current_token_ = RedirectToken::REDIRECT_APPEND;
                original_data_->get();
                return;
            }

            current_token_ = RedirectToken::REDIRECT_OUT;
            return;
        }

        if (static_cast<char>(current_char) == '<') {
            int next_char = original_data_->peek();

            if (next_char == '<') {
                current_token_ = RedirectToken::HERE_DOC;
                original_data_->get();
                return;
            }

            current_token_ = RedirectToken::REDIRECT_IN;
            return;
        }

        // if (static_cast<char>(current_char) == '"') {
        // }

        if (static_cast<unsigned char>(current_char) <= 127) {
            std::string extracted_value = ExtractValue(original_data_, is_end_, current_char);
            current_token_ = WordToken{std::move(extracted_value)};

            return;
        }

        throw std::runtime_error("Find non ASCII symbol");
    }
}