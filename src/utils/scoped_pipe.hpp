#pragma once

#include <array>
#include <format>
#include <string>
#include <unistd.h>
#include <utility>

#include "utils/logger.hpp"
#include "utils/yash_error.hpp"

class ScopedPipe {
public:
    explicit ScopedPipe() {
        raw_read_fd_ = -1;
        raw_write_fd_ = -1;
    };

    ScopedPipe(int raw_read_fd, int raw_write_fd)
        : raw_read_fd_(raw_read_fd)
        , raw_write_fd_(raw_write_fd) {
    }

    ScopedPipe(const ScopedPipe& other) = delete;
    ScopedPipe& operator=(const ScopedPipe& other) = delete;

    ScopedPipe(ScopedPipe&& other) noexcept {
        raw_read_fd_ = std::exchange(other.raw_read_fd_, -1);
        raw_write_fd_ = std::exchange(other.raw_write_fd_, -1);
    };

    ScopedPipe& operator=(ScopedPipe&& other) noexcept {
        if (this != &other) {
            CloseAllRawFD();
            raw_read_fd_ = std::exchange(other.raw_read_fd_, -1);
            raw_write_fd_ = std::exchange(other.raw_write_fd_, -1);
        }

        return *this;
    };

    void CreatePipe() {
        CloseAllRawFD();
        std::array<int, 2> raw_pipe_fd;

        if (pipe(raw_pipe_fd.data()) == -1) {
            throw YashSystemError("TakeNewFD(): Cannot make new pipe");
        }

        raw_read_fd_ = raw_pipe_fd[0];
        raw_write_fd_ = raw_pipe_fd[1];

        LOG_DEBUG(
            "TakeNewFD(): Get current FD: Read={} Write={}", raw_read_fd_, raw_write_fd_

        );
    }

    void CloseRawReadFD() {
        if (raw_read_fd_ != -1) {
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

    ~ScopedPipe() {
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
};