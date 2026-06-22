#include "expander/expander.hpp"
#include "tokenizer/itokenizer.hpp"
#include "tokenizer/tokenizer.hpp"

#include <sstream>
#include <string>
#include <utility>
#include <variant>

[[nodiscard]] bool Expander::IsEnd() const {
    return is_end_;
};

void Expander::RunLogic() {
    if (is_end_ || (base_tokenizer_.IsEnd() && buffer_.empty())) {
        is_end_ = true;
        return;
    }

    if (!buffer_.empty()) {
        current_token_ = std::move(buffer_.front());
        buffer_.pop_front();
        return;
    }

    Token curr_token = base_tokenizer_.GetToken();

    if (std::holds_alternative<PipeToken>(curr_token)) {
        is_command_position_ = true;
        is_waiting_for_redirect_target_ = false;
        current_token_ = std::move(curr_token);
        return;
    }

    if (std::holds_alternative<RedirectToken>(curr_token)) {
        is_waiting_for_redirect_target_ = true;
        current_token_ = std::move(curr_token);
        return;
    }

    if (const WordToken* word_token = std::get_if<WordToken>(&curr_token)) {
        if (is_waiting_for_redirect_target_) {
            is_waiting_for_redirect_target_ = false;
            current_token_ = std::move(curr_token);
            return;
        }

        if (is_command_position_) {
            is_command_position_ = false;

            if (auto it = aliases_.find(word_token->value); it != aliases_.end()) {
                std::istringstream alias_cmd_stream(it->second);

                Tokenizer tokenizer_for_alias{&alias_cmd_stream};

                while (!tokenizer_for_alias.IsEnd()) {
                    buffer_.push_back(tokenizer_for_alias.GetToken());
                    tokenizer_for_alias.Next();
                }

                if (!buffer_.empty()) {
                    current_token_ = buffer_.front();
                    buffer_.pop_front();
                } else {
                    Next();
                }

                return;
            }
        }
    }

    current_token_ = std::move(curr_token);
}

void Expander::Next() {
    if (buffer_.empty()) {
        base_tokenizer_.Next();
    }

    RunLogic();
};

Token Expander::GetToken() {
    return current_token_;
};