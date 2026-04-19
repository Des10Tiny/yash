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

bool IsSpecialChar(char c) {
    return c == '|' || c == '>' || c == '<';
}

enum class State : std::uint8_t { NORMAL, IN_SINGLE, IN_DOUBLE };

std::string ExtractValue(std::istream* data, bool& is_end) {
    std::string result_value;
    State state = State::NORMAL;

    while (true) {
        int curr_char = data->get();

        if (state == State::NORMAL) {
            if (curr_char == EOF) {
                is_end = true;
                break;
            }

            if (std::isspace(static_cast<unsigned char>(curr_char))) {
                break;
            }

            if (IsSpecialChar(static_cast<unsigned char>(curr_char))) {
                data->unget();
                break;
            }

            if (static_cast<unsigned char>(curr_char) == '\'') {
                state = State::IN_SINGLE;
                continue;
            }

            if (static_cast<unsigned char>(curr_char) == '\"') {
                state = State::IN_DOUBLE;
                continue;
            }

            if (std::isprint(static_cast<unsigned char>(curr_char))) {
                result_value += curr_char;
                continue;
            }

            throw std::runtime_error("Find non ASCII symbol");
        }

        else if (state == State::IN_SINGLE) {
            if (curr_char == EOF) {
                throw std::runtime_error("Single quote not closed");
            }

            if (curr_char == '\'') {
                state = State::NORMAL;
                break;
            }

            if (std::isprint(static_cast<unsigned char>(curr_char))) {
                result_value += curr_char;
                continue;
            }

            throw std::runtime_error("Find non ASCII symbol");
        }

        else if (state == State::IN_DOUBLE) {
            if (curr_char == EOF) {
                throw std::runtime_error("Double quote not closed");
            }

            if (curr_char == '\"') {
                state = State::NORMAL;
                break;
            }

            if (std::isprint(static_cast<unsigned char>(curr_char))) {
                result_value += curr_char;
                continue;
            }

            throw std::runtime_error("Find non ASCII symbol");
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

        if (std::isprint(static_cast<unsigned char>(current_char))) {
            original_data_->unget();
            std::string extracted_value = ExtractValue(original_data_, is_end_);
            current_token_ = WordToken{std::move(extracted_value)};

            return;
        }

        throw std::runtime_error("Find non ASCII symbol");
    }
}