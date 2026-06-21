#include "expander/expander.hpp"
#include "tokenizer/itokenizer.hpp"
#include "tokenizer/tokenizer.hpp"

#include <sstream>
#include <string>
#include <variant>

[[nodiscard]] bool Expander::IsEnd() const {
    return is_end_;
};

void Expander::Next() {
    if (is_end_ || base_tokenizer_.IsEnd()) {
        is_end_ = true;
        return;
    }

    if (buffer_.empty()) {
        Token curr_token = base_tokenizer_.GetToken();
        current_token_ = curr_token;

        if (std::holds_alternative<PipeToken>(curr_token)) {
            is_command_position_ = true;
            return;
        }

        if (is_command_position_) {
            if (const WordToken* word_token = std::get_if<WordToken>(&curr_token)) {
                auto it = aliases_.find(word_token->value);

                if (it != aliases_.end()) {
                    std::istringstream alias_cmd_stream(it->second);

                    Tokenizer tokenizer_for_alias{&alias_cmd_stream};

                    while (!tokenizer_for_alias.IsEnd()) {
                        buffer_.push_back(tokenizer_for_alias.GetToken());
                        tokenizer_for_alias.Next();
                    }
                }

                is_command_position_ = false;
            }
        }
    } else {
        current_token_ = buffer_.front();
        buffer_.pop_front();
    }
};

Token Expander::GetToken() {
    return buffer_.empty() ? current_token_ : buffer_.front();
};