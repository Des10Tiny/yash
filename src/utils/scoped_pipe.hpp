#pragma once

#include <array>
#include <format>
#include <unistd.h>
#include <utility>

#include "utils/logger.hpp"

class ScopedPipe {
public:
    explicit ScopedPipe() noexcept {
        raw_read_fd_ = -1;
        raw_write_fd_ = -1;
    };

    ScopedPipe(int raw_read_fd, int raw_write_fd) noexcept
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

    void CreatePipe() noexcept(false) {
        CloseAllRawFD();
        std::array<int, 2> raw_pipe_fd;

        if (pipe(raw_pipe_fd.data()) == -1) {
            throw std::system_error(
                errno, std::generic_category(), "CreatePipe(): Cannot make new pipe"
            );
        }

        raw_read_fd_ = raw_pipe_fd[0];
        raw_write_fd_ = raw_pipe_fd[1];

        LOG_DEBUG(
            "CreatePipe(): Get current FD: Read={} Write={}", raw_read_fd_, raw_write_fd_

        );
    }

    void CloseRawReadFD() noexcept {
        CloseSingleFD(
            raw_read_fd_,
            "CloseRawReadFD()",
            std::format("Cannot close Read File Descriptor read_fd={}", raw_read_fd_)
        );
    }

    void CloseRawWriteFD() noexcept {
        CloseSingleFD(
            raw_write_fd_,
            "CloseRawWriteFD()",
            std::format("Cannot close Write File Descriptor write_fd={}", raw_write_fd_)
        );
    }

    void CloseAllRawFD() noexcept {
        CloseRawReadFD();
        CloseRawWriteFD();
    }

    [[nodiscard]] int GetRawReadFD() const noexcept {
        return raw_read_fd_;
    }

    [[nodiscard]] int GetRawWriteFD() const noexcept {
        return raw_write_fd_;
    }

    ~ScopedPipe() noexcept {
        LOG_DEBUG("~ScopedFD(): ScopedFD - call destructor");
        CloseAllRawFD();
    }

private:
    int raw_read_fd_ = -1;
    int raw_write_fd_ = -1;

    void CloseSingleFD(int& fd, std::string_view label, std::string_view text_for_logger) noexcept {
        if (fd != -1) {
            if (close(fd) == -1) {
                LOG_WARN("{}: {}", label, text_for_logger);
            }

            fd = -1;
        }
    }
};