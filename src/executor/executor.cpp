#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <cstdlib>

#include "executor.hpp"
#include "parser/parser.hpp"
#include "utils/logger.hpp"
#include "utils/yash_error.hpp"
#include "utils/scoped_fd.hpp"

Executor::Executor() {
    builtins_["cd"] = [this](const Command& cmd) { return RunChangeDirectory(cmd); };
    builtins_["exit"] = [this](const Command& cmd) { return RunExit(cmd); };
}

int Executor::RunPipeline(Pipeline& pipeline) {
    if (pipeline.commands.empty() || pipeline.commands[0].args.empty()) {
        return 0;
    }

    if (auto it_is_in_builtins = builtins_.find(pipeline.commands[0].args[0]);
        it_is_in_builtins != builtins_.end() && pipeline.commands.size() == 1) {

        LOG_DEBUG("Command: \'" + it_is_in_builtins->first + "\' find in builtins");
        return it_is_in_builtins->second(pipeline.commands[0]);

    } else {
        int curr_size = 0;
        int size_of_pipline = pipeline.commands.size();
        std::vector<pid_t> all_children_to_wait;
        all_children_to_wait.reserve(size_of_pipline);

        ScopedFD prev_read_fd{true};

        for (Command& i : pipeline.commands) {
            ScopedFD pipe{true};

            if (curr_size != size_of_pipline - 1) {
                LOG_DEBUG(std::string("Executor(): Ask new fd")

                );
                pipe.TakeNewFD();
                LOG_DEBUG(std::string("Get new fd") +
                          "Read=" + std::to_string(pipe.GetRawReadFD()) +
                          " Write=" + std::to_string(pipe.GetRawWriteFD())

                );
            }

            LOG_DEBUG(std::string("Make new pipe ") +
                      "Read=" + std::to_string(pipe.GetRawReadFD()) +
                      " Write=" + std::to_string(pipe.GetRawWriteFD())

            );

            pid_t curr_pid = fork();

            if (curr_pid < 0) {
                throw YashSystemError("Cannot make new fork");
            }

            if (curr_pid == 0) {
                if (prev_read_fd.GetRawReadFD() != -1) {
                    int dup2_read_status = dup2(prev_read_fd.GetRawReadFD(), STDIN_FILENO);

                    LOG_DEBUG("Child made dup2 for read"

                    );

                    if (dup2_read_status < 0) {
                        throw YashSystemError("dup2 create read failed");
                    }
                }

                if (pipe.IsBothCorrect()) {
                    int dup2_write_status = dup2(pipe.GetRawWriteFD(), STDOUT_FILENO);
                    LOG_DEBUG("Child made dup2 for write"

                    );

                    if (dup2_write_status < 0) {
                        throw YashSystemError("dup2 create write failed");
                    }
                }

                pipe.CloseAllRawFD();
                LOG_DEBUG("Child close all parent pipe"

                );

                prev_read_fd.CloseRawReadFD();
                std::vector<char*> char_vector = CharFromVectorHandler(i.args);
                execvp(char_vector[0], char_vector.data());

                switch (errno) {
                    case EACCES: {
                        LOG_DEBUG(
                            std::string("Command \'") + char_vector[0] + "\'. Permission denied"

                        );

                        std::cerr << std::string("Command \'") + char_vector[0] +
                                         "\'. Permission denied "
                                  << '\n';

                        std::_Exit(ExitCode::PERMISSION_DENIED);
                    }

                    case ENOENT: {
                        LOG_DEBUG(std::string("Command \'") + char_vector[0] + "\' not found"

                        );
                        std::cerr << std::string("Command \'") + char_vector[0] + "\' not found"
                                  << '\n';
                        std::_Exit(ExitCode::COMMAND_NOT_FOUND);
                    }

                    default: {
                        LOG_WARN(std::string("Command \'") + char_vector[0] + "\' execution failed"

                        );
                        std::cerr << "yash: execution failed: " << char_vector[0] << '\n';
                        std::_Exit(ExitCode::GENERAL_FAILURE);
                    }
                }

            } else {
                LOG_DEBUG(std::string("Prev read FD before move Read=") +
                          std::to_string(prev_read_fd.GetRawReadFD()) +
                          " Write=" + std::to_string(prev_read_fd.GetRawWriteFD())

                );
                prev_read_fd = std::move(pipe);
                LOG_DEBUG(std::string("Prev read FD after move Read=") +
                          std::to_string(prev_read_fd.GetRawReadFD()) +
                          " Write=" + std::to_string(prev_read_fd.GetRawWriteFD())

                );

                prev_read_fd.CloseRawWriteFD();
                all_children_to_wait.push_back(curr_pid);
            }

            curr_size++;
        }

        return WaitForAllChildren(all_children_to_wait);
    }

    return -1;
}

int Executor::WaitForAllChildren(const std::vector<pid_t>& all_children_to_wait) {
    int last_status = 0;

    for (pid_t curr_child_pid : all_children_to_wait) {
        int status;

        if (waitpid(curr_child_pid, &status, 0) == -1) {
            throw YashSystemError("Waitpid failed");
        }

        if (WIFEXITED(status)) {
            LOG_DEBUG("Process pid=" + std::to_string(curr_child_pid) +
                      " ended with the code: " + std::to_string(WEXITSTATUS(status))

            );
        } else if (WIFSIGNALED(status)) {
            LOG_WARN("Process pid= " + std::to_string(curr_child_pid) +
                     " was killed by a signal: " + std::to_string(WTERMSIG(status))

            );
        }
        last_status = status;
    }

    return last_status;
}