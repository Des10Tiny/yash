#pragma once

#include <array>
#include <format>
#include <unistd.h>
#include <utility>

#include "utils/logger.hpp"

/**
 * RAII wrapper managing a unidirectional POSIX pipe (`read` and `write` file descriptors)
 *
 * Ensures that both ends of the pipe are properly closed upon destruction or reassignment,
 * preventing descriptor leaks across processes and pipeline stages Non-copyable, movable
 */
class ScopedPipe {
public:
    /**
     * Constructs an uninitialized `ScopedPipe` with invalid file descriptors (`-1`)
     */
    explicit ScopedPipe() noexcept {
        raw_read_fd_ = -1;
        raw_write_fd_ = -1;
    };

    /**
     * Constructs a `ScopedPipe` by taking ownership of existing raw descriptors
     *
     * @param raw_read_fd Descriptor for the read end of the pipe
     * @param raw_write_fd Descriptor for the write end of the pipe
     */
    ScopedPipe(int raw_read_fd, int raw_write_fd) noexcept
        : raw_read_fd_(raw_read_fd)
        , raw_write_fd_(raw_write_fd) {
    }

    ScopedPipe(const ScopedPipe& other) = delete;
    ScopedPipe& operator=(const ScopedPipe& other) = delete;

    /**
     * Move constructor Transfers ownership of both descriptors, resetting source to `-1`
     */
    ScopedPipe(ScopedPipe&& other) noexcept {
        raw_read_fd_ = std::exchange(other.raw_read_fd_, -1);
        raw_write_fd_ = std::exchange(other.raw_write_fd_, -1);
    };

    /**
     * Move assignment operator Closes current descriptors and takes ownership from source
     */
    ScopedPipe& operator=(ScopedPipe&& other) noexcept {
        if (this != &other) {
            CloseAllRawFD();
            raw_read_fd_ = std::exchange(other.raw_read_fd_, -1);
            raw_write_fd_ = std::exchange(other.raw_write_fd_, -1);
        }

        return *this;
    };

    /**
     * Closes existing descriptors and creates a new OS pipe via `pipe()`
     *
     * `return` `true` if the pipe was created successfully, `false` on system failure
     */
    [[nodiscard("You must verify if pipe was actually created before using it")]] bool
    CreatePipe() noexcept {
        CloseAllRawFD();
        std::array<int, 2> raw_pipe_fd;

        if (pipe(raw_pipe_fd.data()) == -1) {
            LOG_WARN("CreatePipe(): failed to create pipe: {}", std::strerror(errno));
            return false;
        }

        raw_read_fd_ = raw_pipe_fd[0];
        raw_write_fd_ = raw_pipe_fd[1];

        LOG_DEBUG(
            "CreatePipe(): Get current FD: Read={} Write={}", raw_read_fd_, raw_write_fd_

        );
        return true;
    }

    /**
     * Closes the read end of the pipe if open, resetting its descriptor to `-1`
     */
    void CloseRawReadFD() noexcept {
        CloseSingleFD(
            raw_read_fd_,
            "CloseRawReadFD()",
            std::format("Cannot close Read File Descriptor read_fd={}", raw_read_fd_)
        );
    }

    /**
     * Closes the write end of the pipe if open, resetting its descriptor to `-1`
     */
    void CloseRawWriteFD() noexcept {
        CloseSingleFD(
            raw_write_fd_,
            "CloseRawWriteFD()",
            std::format("Cannot close Write File Descriptor write_fd={}", raw_write_fd_)
        );
    }

    /**
     * Closes both read and write ends of the pipe if open
     */
    void CloseAllRawFD() noexcept {
        CloseRawReadFD();
        CloseRawWriteFD();
    }

    /**
     * Retrieves the raw read file descriptor without transferring ownership
     *
     * `return` The read descriptor integer, or `-1` if uninitialized/closed
     */
    [[nodiscard]] int GetRawReadFD() const noexcept {
        return raw_read_fd_;
    }

    /**
     * Retrieves the raw write file descriptor without transferring ownership
     *
     * `return` The write descriptor integer, or `-1` if uninitialized/closed
     */
    [[nodiscard]] int GetRawWriteFD() const noexcept {
        return raw_write_fd_;
    }

    /**
     * Destructor
     * Automatically closes any remaining open file descriptors
     */
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
