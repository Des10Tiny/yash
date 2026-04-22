#include "parser.hpp"
#include <stdexcept>
#include <variant>
#include "tokenizer/tokenizer.hpp"

std::optional<Pipeline> Parser::ParsePipline() {
    tokenizer_.Next();
    return std::nullopt;
}

std::optional<Command> Parser::ParseCommand() {
    Command cmd;

    while (!tokenizer_.IsEnd() && (std::holds_alternative<WordToken>(tokenizer_.GetToken()) ||
                                   std::holds_alternative<RedirectToken>(tokenizer_.GetToken()))) {

        Token curr_token = tokenizer_.GetToken();

        if (WordToken* word_token = std::get_if<WordToken>(&curr_token)) {
            cmd.args.emplace_back(std::move(word_token->value));
            tokenizer_.Next();

        } else if (RedirectToken* redirect_token = std::get_if<RedirectToken>(&curr_token)) {
            RedirectToken type = *redirect_token;

            tokenizer_.Next();

            if (tokenizer_.IsEnd()) {
                throw std::runtime_error(
                    "Syntax error:\nExpected filename after redirect, got EOF");
            }

            curr_token = tokenizer_.GetToken();

            if (WordToken* word_file_token = std::get_if<WordToken>(&curr_token)) {
                if (type == RedirectToken::REDIRECT_IN) {
                    cmd.redirect_in = std::move(word_file_token->value);

                } else if (type == RedirectToken::REDIRECT_OUT) {
                    cmd.redirect_out = std::move(word_file_token->value);
                    cmd.append_out = false;

                } else if (type == RedirectToken::REDIRECT_APPEND) {
                    cmd.redirect_out = std::move(word_file_token->value);
                    cmd.append_out = true;

                } else if (type == RedirectToken::HERE_DOC) {
                    throw std::runtime_error("Not implemented");
                }

                tokenizer_.Next();
            } else {
                throw std::runtime_error(
                    "Syntax error:\nExpected filename after redirect, got unexpected token");
            }
        }
    }

    if (cmd.args.empty() && cmd.redirect_in.empty() && cmd.redirect_out.empty()) {
        return std::nullopt;
    }

    return cmd;
}