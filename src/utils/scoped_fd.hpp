#pragma once

#include <array>
#include <format>
#include <string>
#include <unistd.h>
#include <utility>

#include "utils/logger.hpp"
#include "utils/yash_error.hpp"

class ScopedFD {
public:
    ScopedFD() {
        std::array<int, 2> raw_pipe_fd;

        if (pipe(raw_pipe_fd.data()) == -1) {
            throw YashSystemError("ScopedFD(): Cannot make new pipe");
        }

        raw_read_fd_ = raw_pipe_fd[0];
        raw_write_fd_ = raw_pipe_fd[1];
        is_both_fd_correct_ = true;

        LOG_DEBUG(
            "ScopedFD(): Get current FD: Read={} Write={}", raw_read_fd_, raw_write_fd_

        );
    }

    explicit ScopedFD(bool is_need_to_be_empty) {
        if (is_need_to_be_empty) {
            raw_read_fd_ = -1;
            raw_write_fd_ = -1;
            is_both_fd_correct_ = false;
        } else {
            LOG_WARN("ScopedFD(bool is_need_to_be_emty): You doing something nasty");
        }
    };

    ScopedFD(int raw_read_fd, int raw_write_fd)
        : raw_read_fd_(raw_read_fd)
        , raw_write_fd_(raw_write_fd) {
    }

    ScopedFD(const ScopedFD& other) = delete;
    ScopedFD& operator=(const ScopedFD& other) = delete;

    ScopedFD(ScopedFD&& other) noexcept {
        raw_read_fd_ = std::exchange(other.raw_read_fd_, -1);
        raw_write_fd_ = std::exchange(other.raw_write_fd_, -1);
        is_both_fd_correct_ = std::exchange(other.is_both_fd_correct_, false);
    };

    ScopedFD& operator=(ScopedFD&& other) noexcept {
        if (this != &other) {
            CloseAllRawFD();
            raw_read_fd_ = std::exchange(other.raw_read_fd_, -1);
            raw_write_fd_ = std::exchange(other.raw_write_fd_, -1);
            is_both_fd_correct_ = true;
        }

        return *this;
    };

    void TakeNewFD() {
        CloseAllRawFD();
        std::array<int, 2> raw_pipe_fd;

        if (pipe(raw_pipe_fd.data()) == -1) {
            throw YashSystemError("TakeNewFD(): Cannot make new pipe");
        }

        raw_read_fd_ = raw_pipe_fd[0];
        raw_write_fd_ = raw_pipe_fd[1];
        is_both_fd_correct_ = true;

        LOG_DEBUG(
            "TakeNewFD(): Get current FD: Read={} Write={}", raw_read_fd_, raw_write_fd_

        );
    }

    void CloseRawReadFD() {
        if (raw_read_fd_ != -1) {
            is_both_fd_correct_ = false;

            if (close(raw_read_fd_) == -1) {
                throw YashSystemError(
                    std::format(
                        "CloseRawReadFD(): Cannot close Read File Descriptor read_fd={}",
                        raw_read_fd_
                    )
                );
            }
            raw_read_fd_ = -1;
        }
    }

    void CloseRawWriteFD() {
        if (raw_write_fd_ != -1) {
            is_both_fd_correct_ = false;

            if (close(raw_write_fd_) == -1) {
                throw YashSystemError(
                    std::format(
                        "CloseRawWriteFD(): Cannot close Write File Descriptor write_fd={}",
                        raw_write_fd_
                    )
                );
            }
            raw_write_fd_ = -1;
        }
    }

    void CloseAllRawFD() {
        is_both_fd_correct_ = false;

        if (raw_read_fd_ != -1) {
            if (close(raw_read_fd_) == -1) {
                LOG_WARN(
                    "CloseAllRawFD(): Cannot close Read File Descriptor read_fd={}", raw_read_fd_

                );
            }
            raw_read_fd_ = -1;
        }

        if (raw_write_fd_ != -1) {
            if (close(raw_write_fd_) == -1) {
                LOG_WARN(
                    "CloseAllRawFD(): Cannot close Write File Descriptor write_fd={}", raw_write_fd_
                );
            }
            raw_write_fd_ = -1;
        }
    }

    [[nodiscard]] int GetRawReadFD() const {
        return raw_read_fd_;
    }

    [[nodiscard]] int GetRawWriteFD() const {
        return raw_write_fd_;
    }

    [[nodiscard]] bool IsBothCorrect() const {
        return is_both_fd_correct_;
    }

    ~ScopedFD() {
        LOG_DEBUG("~ScopedFD(): ScopedFD - call destructor");

        if (raw_read_fd_ != -1) {
            if (close(raw_read_fd_) == -1) {
                LOG_WARN(
                    "~ScopedFD(): Cannot close Read File Descriptor read_fd={}", raw_read_fd_

                );
            }
        }

        if (raw_write_fd_ != -1) {
            if (close(raw_write_fd_) == -1) {
                LOG_WARN(
                    "~ScopedFD(): Cannot close Write File Descriptor write_fd={}", raw_write_fd_
                );
            }
        }
    }

private:
    int raw_read_fd_ = -1;
    int raw_write_fd_ = -1;
    bool is_both_fd_correct_ = true;
};