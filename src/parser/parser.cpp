#include "parser.hpp"

#include <optional>
#include <variant>

#include "tokenizer/tokenizer.hpp"
#include "utils/yash_error.hpp"

std::optional<Pipeline> Parser::ParsePipeline() {
    Pipeline pipeline;

    if (!tokenizer_.IsEnd() && std::holds_alternative<PipeToken>(tokenizer_.GetToken())) {
        throw YashSyntaxError("Unexpected token '|'");
    }

    std::optional<Command> cmd = ParseCommand();

    if (cmd.has_value()) {
        pipeline.commands.push_back(cmd.value());

        while (!tokenizer_.IsEnd()) {

            if (std::holds_alternative<PipeToken>(tokenizer_.GetToken())) {

                tokenizer_.Next();
                std::optional<Command> next_cmd = ParseCommand();

                if (!next_cmd.has_value()) {
                    throw YashSyntaxError("Expected command after pipe, got EOF");
                }
                pipeline.commands.push_back(next_cmd.value());

            } else {
                break;
            }
        }

        return pipeline;
    }

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
                throw YashSyntaxError("Expected filename after redirect, got EOF");
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
                    throw YashSyntaxError("Not implemented");
                }

                tokenizer_.Next();
            } else {
                throw YashSyntaxError("Expected filename after redirect, got unexpected token");
            }
        }
    }

    if (cmd.args.empty() && cmd.redirect_in.empty() && cmd.redirect_out.empty()) {
        return std::nullopt;
    }

    return cmd;
}