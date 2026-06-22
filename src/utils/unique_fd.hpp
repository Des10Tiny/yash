#pragma once

#include "utils/logger.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <utility>

class UniqueFD {
public:
    explicit UniqueFD() noexcept {
        opened_fd_ = -1;
    }

    explicit UniqueFD(int opened_fd) noexcept
        : opened_fd_(opened_fd) {
    }

    UniqueFD(const UniqueFD& other) = delete;
    UniqueFD& operator=(const UniqueFD& other) = delete;

    UniqueFD(UniqueFD&& other) noexcept {
        opened_fd_ = std::exchange(other.opened_fd_, -1);
    };

    UniqueFD& operator=(UniqueFD&& other) noexcept {
        if (this != &other) {
            CloseFD();
            opened_fd_ = std::exchange(other.opened_fd_, -1);
        }

        return *this;
    };

    ~UniqueFD() noexcept {
        LOG_DEBUG("~UniqueFD(): ScopedFD - call destructor");
        CloseFD();
    }

    void CloseFD() noexcept {
        if (opened_fd_ != -1) {
            if (close(opened_fd_) == -1) {
                LOG_WARN(
                    "UniqueFD(): Cannot close File Descriptor fd={}", opened_fd_

                );
            }
        }
        opened_fd_ = -1;
    }

    template <typename... Args>
    [[nodiscard("You must verify if FD was actually created before using it")]] bool
    CreateNewFD(Args&&... args) noexcept {
        CloseFD();

        int fd = open(std::forward<Args>(args)...);

        if (fd == -1) {
            LOG_WARN("CreateFD(): Cannot make new fd");
            return false;
        }

        opened_fd_ = fd;
        LOG_DEBUG("CreateNewFD Get new fd={}", opened_fd_);

        return true;
    }

    [[nodiscard]] int GetRawFD() const noexcept {
        return opened_fd_;
    }

private:
    int opened_fd_ = -1;
};