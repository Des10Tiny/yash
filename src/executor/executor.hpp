#pragma once

#include <functional>
#include <unordered_map>
#include "parser/parser.hpp"

class Executor {
public:
    Executor();
    int RunPipeline(Pipeline& pipeline);

private:
    std::unordered_map<std::string, std::function<int(const Command&)>> builtins_;

    int RunChangeDirectory([[maybe_unused]] const Command& cmd) {
        return 1;
    }

    int RunExit([[maybe_unused]] const Command& cmd) {
        return 1;
    }

    int WaitForAllChildren(const std::vector<pid_t>& children);

    std::vector<char*> CharFromVectorHandler(std::vector<std::string>& args) {
        std::vector<char*> c_args;
        c_args.reserve(args.size() + 1);

        for (auto& arg : args) {
            c_args.push_back(arg.data());
        }

        c_args.push_back(nullptr);
        return c_args;
    }
};