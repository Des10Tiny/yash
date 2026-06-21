#include "tokenizer/itokenizer.hpp"
#include "tokenizer/tokenizer.hpp"

#include <unordered_map>

class Expander : public ITokenizer {
public:
    Expander(
        const Tokenizer& base_tokenizer, const std::unordered_map<std::string, std::string>& aliases
    )
        : base_tokenizer_(base_tokenizer)
        , aliases_(aliases) {

        };

    ~Expander() override = default;

    [[nodiscard]] bool IsEnd() const override;

    void Next() override;

    Token GetToken() override;

private:
    const Tokenizer& base_tokenizer_;
    std::unordered_map<std::string, std::string> aliases_;
};