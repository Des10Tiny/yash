#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "executor.hpp"
#include "parser/parser.hpp"
#include "utils/logger.hpp"
#include "utils/scoped_pipe.hpp"
#include "utils/unique_fd.hpp"
#include "utils/yash_error.hpp"

Executor::Executor() {
    builtins_["cd"] = [this](const Command& cmd) { return RunChangeDirectory(cmd); };
    builtins_["exit"] = [this](const Command& cmd) { return RunExit(cmd); };
}

int Executor::RunPipeline(Pipeline& pipeline) {
    if (pipeline.commands.empty() || pipeline.commands[0].args.empty()) {
        return 0;
    }

    if (auto it_is_in_builtins = builtins_.find(pipeline.commands[0].args[0]);
        it_is_in_builtins != builtins_.end()) {
        if (pipeline.commands.size() > 1) {
            throw YashBuiltinError("builtins cannot be used in pipelines");
        }

        LOG_DEBUG("Command: \'{}\' find in builtins", it_is_in_builtins->first);
        return it_is_in_builtins->second(pipeline.commands[0]);

    } else {
        int curr_size = 0;
        int size_of_pipline = pipeline.commands.size();
        std::vector<pid_t> all_children_to_wait;
        all_children_to_wait.reserve(size_of_pipline);

        ScopedPipe prev_read_fd;

        for (Command& i : pipeline.commands) {
            ScopedPipe pipe;

            if (curr_size != size_of_pipline - 1) {
                LOG_DEBUG(
                    "Executor(): Ask new fd"

                );
                pipe.CreatePipe();
                LOG_DEBUG(
                    "Get new fd Read={} Write={}", pipe.GetRawReadFD(), pipe.GetRawWriteFD()

                );
            }

            LOG_DEBUG(
                "Make new pipe Read={} Write={}", pipe.GetRawReadFD(), pipe.GetRawWriteFD()

            );

            pid_t curr_pid = fork();

            if (curr_pid < 0) {
                throw YashSystemError("Cannot make new fork");
            }

            if (curr_pid == 0) {
                if (i.args.empty()) {
                    LOG_DEBUG("Executor(): Command is empty");
                    std::_Exit(0);
                }

                if (prev_read_fd.GetRawReadFD() != -1) {
                    LOG_DEBUG(
                        "Child made dup2 for read"

                    );

                    if (dup2(prev_read_fd.GetRawReadFD(), STDIN_FILENO) < 0) {
                        std::cerr << std::format(
                                         "yash: pipe: dup2 create read failed: {}",
                                         std::strerror(errno)
                                     )
                                  << '\n';

                        std::_Exit(1);
                    }
                }

                if (pipe.GetRawReadFD() != -1 && pipe.GetRawWriteFD() != -1) {
                    LOG_DEBUG(
                        "Child made dup2 for write"

                    );

                    if (dup2(pipe.GetRawWriteFD(), STDOUT_FILENO) < 0) {
                        std::cerr << std::format(
                                         "yash: pipe: dup2 create write failed: {}",
                                         std::strerror(errno)
                                     )
                                  << '\n';

                        std::_Exit(1);
                    }
                }

                pipe.CloseAllRawFD();
                LOG_DEBUG(
                    "Child close all parent pipe"

                );

                prev_read_fd.CloseRawReadFD();
                std::vector<char*> char_vector = CharFromVectorHandler(i.args);

                if (!i.redirect_in.empty()) {
                    UniqueFD redirect_in;
                    if (!redirect_in.CreateNewFD(i.redirect_in.data(), O_RDONLY)) {
                        std::cerr << std::format(
                                         "yash: cannot make new fd:{}", std::strerror(errno)
                                     )
                                  << '\n';
                        std::_Exit(1);
                    }

                    if (dup2(redirect_in.GetRawFD(), STDIN_FILENO) < 0) {
                        std::cerr << std::format("yash: redirection error:{}", std::strerror(errno))
                                  << '\n';
                        std::_Exit(1);
                    }
                }

                if (!i.redirect_out.empty()) {
                    UniqueFD redirect_out;

                    if (i.append_out) {
                        if (!redirect_out.CreateNewFD(
                                i.redirect_out.data(), O_WRONLY | O_CREAT | O_APPEND, 0666
                            )) {
                            std::cerr
                                << std::format("yash: cannot make new fd:{}", std::strerror(errno))
                                << '\n';
                            std::_Exit(1);
                        }
                    } else {
                        if (!redirect_out.CreateNewFD(
                                i.redirect_out.data(), O_WRONLY | O_CREAT | O_TRUNC, 0666
                            )) {
                            std::cerr
                                << std::format("yash: cannot make new fd:{}", std::strerror(errno))
                                << '\n';
                            std::_Exit(1);
                        }
                    }
                    if (dup2(redirect_out.GetRawFD(), STDOUT_FILENO) < 0) {
                        std::cerr << std::format("yash: redirection error:{}", std::strerror(errno))
                                  << '\n';
                        std::_Exit(1);
                    }
                }

                execvp(char_vector[0], char_vector.data());
                HandleExecFailure(char_vector[0], errno);

            } else {
                LOG_DEBUG(
                    "Prev read FD before move Read={} Write={}",
                    prev_read_fd.GetRawReadFD(),
                    prev_read_fd.GetRawWriteFD()

                );
                prev_read_fd = std::move(pipe);
                LOG_DEBUG(
                    "Prev read FD after move Read={} Write={}",
                    prev_read_fd.GetRawReadFD(),
                    prev_read_fd.GetRawWriteFD()

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
            LOG_DEBUG(
                "Process pid={} ended with the code={}", curr_child_pid, WEXITSTATUS(status)

            );

            last_status = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            LOG_WARN(
                "Process pid={} was killed by a signal={} ", curr_child_pid, WTERMSIG(status)

            );
            last_status = 128 + WTERMSIG(status);
        }
    }

    return last_status;
}

[[noreturn]] void Executor::HandleExecFailure(const std::string& cmd_name, int err_code) {
    LOG_DEBUG("execvp failed for '{}', errno: {}", cmd_name, err_code);

    std::unique_ptr<YashError> error;

    switch (err_code) {
    case EACCES: {
        error = std::make_unique<YashPermissionError>(cmd_name);
        LOG_DEBUG(
            "Command \'{}\'. Permission denied", cmd_name

        );
        break;
    }
    case ENOENT: {
        error = std::make_unique<YashCommandNotFoundError>(cmd_name);
        LOG_DEBUG(
            "Command \'{}\' not found", cmd_name

        );
        break;
    }
    default: {
        error =
            std::make_unique<YashSystemError>(std::format("execution failed for \'{}\'", cmd_name));
        LOG_WARN(
            "Command \'{}\' execution failed", cmd_name

        );
        break;
    }
    }

    std::cerr << error->what() << '\n';

    std::_Exit(error->GetCode());
}

int Executor::RunChangeDirectory(const Command& cmd) {
    if (cmd.args.size() == 1) {
        const char* home_value = getenv("HOME");

        if (!home_value) {
            throw YashBuiltinError("cd: HOME not set");
        }

        if (chdir(home_value) == -1) {
            throw YashBuiltinError(std::format("cd: {}: {}", home_value, std::strerror(errno)));
        }

    } else if (cmd.args.size() == 2) {
        if (chdir(cmd.args[1].c_str()) == -1) {
            throw YashBuiltinError(std::format("cd: {}: {}", cmd.args[1], std::strerror(errno)));
        }
    } else {
        throw YashBuiltinError("cd: too many arguments");
    }

    return ExitCode::SUCCESS;
}

[[noreturn]] int Executor::RunExit(const Command& cmd) {
    if (cmd.args.size() == 1) {
        throw YashExitException(0);

    } else if (cmd.args.size() == 2) {
        try {
            int code = std::stoi(cmd.args[1]);
            throw YashExitException(code);
        } catch (const std::invalid_argument& e) {
            throw YashBuiltinError(std::format("exit: {}: numeric argument required", cmd.args[1]));
        } catch (const std::out_of_range& e) {
            throw YashBuiltinError(std::format("exit: {}: number out of range", cmd.args[1]));
        }
    }

    throw YashBuiltinError("exit: too many arguments");
}