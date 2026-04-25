#include <functional>
#include <unordered_map>
#include "parser/parser.hpp"

class Executor {

private:
    std::unordered_map<std::string, std::function<int(const Command&)>> builtins_;
};