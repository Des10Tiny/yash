#include "expander/expander.hpp"
#include "tokenizer/itokenizer.hpp"

[[nodiscard]] bool Expander::IsEnd() const {
    return true;
};

void Expander::Next() {};

Token Expander::GetToken() {
    Token a;
    return a;
};