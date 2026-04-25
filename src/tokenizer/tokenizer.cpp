#include "tokenizer.hpp"

#include <cctype>
#include <cstdio>
#include <string>

#include "utils/yash_error.hpp"

namespace {
enum class State : std::uint8_t { NORMAL, IN_SINGLE, IN_DOUBLE };

bool IsSpecialChar(char c) {
    return c == '|' || c == '>' || c == '<';
}

std::string ExtractValue(std::istream* data) {
    std::string result_value;
    State state = State::NORMAL;

    while (true) {
        int curr_char = data->get();

        if (curr_char == EOF) {
            if (state != State::NORMAL) {
                throw YashSyntaxError("Quote not closed");
            }
            break;
        }

        if (!std::isprint(curr_char) && !std::isspace(curr_char)) {
            throw YashSyntaxError("Found non-printable symbol");
        }

        if (state == State::NORMAL) {
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

            result_value += curr_char;
        }

        else if (state == State::IN_SINGLE) {
            if (curr_char == '\'') {
                state = State::NORMAL;
            } else {
                result_value += curr_char;
            }
        }

        else if (state == State::IN_DOUBLE) {

            if (curr_char == '\"') {
                state = State::NORMAL;
            } else {
                result_value += curr_char;
            }
        }
    }

    return result_value;
}
}  // namespace

Tokenizer::Tokenizer(std::istream* in) : original_data_(in) {
    Next();
}

bool Tokenizer::IsEnd() {
    return is_end_;
}

Token Tokenizer::GetToken() {
    return current_token_;
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
            if (original_data_->peek() == '>') {
                current_token_ = RedirectToken::REDIRECT_APPEND;
                original_data_->get();
            } else {
                current_token_ = RedirectToken::REDIRECT_OUT;
            }

            return;
        }

        if (static_cast<char>(current_char) == '<') {
            if (original_data_->peek() == '<') {
                current_token_ = RedirectToken::HERE_DOC;
                original_data_->get();
            } else {
                current_token_ = RedirectToken::REDIRECT_IN;
            }

            return;
        }

        original_data_->unget();
        std::string extracted_value = ExtractValue(original_data_);
        current_token_ = WordToken{std::move(extracted_value)};
        return;
    }
}