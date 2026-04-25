#include <unistd.h>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <array>

class ScopedFD {
    ScopedFD() {
        std::array<int, 2> raw_pipe_fd;

        if (pipe(raw_pipe_fd.data()) == -1) {
            throw std::runtime_error("Critical failure:\nCannot make new pipe");
        }

        raw_read_fd_ = raw_pipe_fd[0];
        raw_write_fd_ = raw_pipe_fd[1];
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
            CloseRawReadFD();
            CloseRawWriteFD();
            raw_read_fd_ = std::exchange(other.raw_read_fd_, -1);
            raw_write_fd_ = std::exchange(other.raw_write_fd_, -1);
        }

        return *this;
    };

    void CloseRawReadFD() {
        if (raw_read_fd_ != -1) {
            if (close(raw_read_fd_) == -1) {
                throw std::runtime_error("Critical failure:\nCannot close Read File Descriptor");
            }
            raw_read_fd_ = -1;
        }
    }

    void CloseRawWriteFD() {
        if (raw_write_fd_ != -1) {
            if (close(raw_write_fd_) == -1) {
                throw std::runtime_error("Critical failure:\nCannot close Write File Descriptor");
            }
            raw_write_fd_ = -1;
        }
    }

    int GetRawReadFD() {
        return raw_read_fd_;
    }

    int GetRawWriteFD() {
        return raw_write_fd_;
    }

    ~ScopedFD() {
        if (raw_read_fd_ != -1) {
            if (close(raw_read_fd_) == -1) {
                std::cerr << ("Critical failure:\nCannot close Read File Descriptor") << '\n';
            }
        }

        if (raw_write_fd_ != -1) {
            if (close(raw_write_fd_) == -1) {
                std::cerr << ("Critical failure:\nCannot close Read Write Descriptor") << '\n';
            }
        }
    }

private:
    int raw_read_fd_ = -1;
    int raw_write_fd_ = -1;
};