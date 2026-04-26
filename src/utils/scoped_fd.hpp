#pragma once

#include <unistd.h>
#include <string>
#include <utility>
#include <array>

#include "utils/logger.hpp"
#include "utils/yash_error.hpp"

class ScopedFD {
public:
    ScopedFD() {
        std::array<int, 2> raw_pipe_fd;

        if (pipe(raw_pipe_fd.data()) == -1) {
            throw YashSystemError("Cannot make new pipe");
        }

        raw_read_fd_ = raw_pipe_fd[0];
        raw_write_fd_ = raw_pipe_fd[1];

        LOG_DEBUG(std::string("Get current FD: Read=") + std::to_string(raw_read_fd_) +
                  " Write=" + std::to_string(raw_write_fd_)

        );
    }

    ScopedFD(int raw_read_fd, int raw_write_fd)
        : raw_read_fd_(raw_read_fd), raw_write_fd_(raw_write_fd) {
    }

    ScopedFD(const ScopedFD& other) = delete;
    ScopedFD& operator=(const ScopedFD& other) = delete;

    ScopedFD(ScopedFD&& other) noexcept {
        raw_read_fd_ = std::exchange(other.raw_read_fd_, -1);
        raw_write_fd_ = std::exchange(other.raw_write_fd_, -1);
    };

    ScopedFD& operator=(ScopedFD&& other) noexcept {
        if (this != &other) {
            CloseAllRawFD();
            raw_read_fd_ = std::exchange(other.raw_read_fd_, -1);
            raw_write_fd_ = std::exchange(other.raw_write_fd_, -1);
        }

        return *this;
    };

    void CloseRawReadFD() {
        if (raw_read_fd_ != -1) {
            is_both_fd_correct_ = false;

            if (close(raw_read_fd_) == -1) {
                throw YashSystemError("Cannot close Read File Descriptor: " +
                                      std::to_string(raw_read_fd_));
            }
            raw_read_fd_ = -1;
        }
    }

    void CloseRawWriteFD() {
        if (raw_write_fd_ != -1) {
            is_both_fd_correct_ = false;

            if (close(raw_write_fd_) == -1) {
                throw YashSystemError("Cannot close Write File Descriptor: " +
                                      std::to_string(raw_write_fd_));
            }
            raw_write_fd_ = -1;
        }
    }

    void CloseAllRawFD() {
        is_both_fd_correct_ = false;

        if (raw_read_fd_ != -1) {
            if (close(raw_read_fd_) == -1) {
                LOG_WARN("Cannot close Read File Descriptor: " + std::to_string(raw_read_fd_));
            }
            raw_read_fd_ = -1;
        }

        if (raw_write_fd_ != -1) {
            if (close(raw_write_fd_) == -1) {
                LOG_WARN("Cannot close Write File Descriptor: " + std::to_string(raw_write_fd_));
            }
            raw_write_fd_ = -1;
        }
    }

    int GetRawReadFD() const {
        return raw_read_fd_;
    }

    int GetRawWriteFD() const {
        return raw_write_fd_;
    }

    bool IsBothCorrect() const {
        return is_both_fd_correct_;
    }

    ~ScopedFD() {
        LOG_DEBUG("ScopedFD - call destructor");

        if (raw_read_fd_ != -1) {
            if (close(raw_read_fd_) == -1) {
                LOG_WARN("Cannot close Read File Descriptor: " + std::to_string(raw_read_fd_));
            }
        }

        if (raw_write_fd_ != -1) {
            if (close(raw_write_fd_) == -1) {
                LOG_WARN("Cannot close Write File Descriptor: " + std::to_string(raw_write_fd_));
            }
        }
    }

private:
    int raw_read_fd_ = -1;
    int raw_write_fd_ = -1;
    bool is_both_fd_correct_ = true;
};