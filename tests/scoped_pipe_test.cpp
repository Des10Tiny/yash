#include <fcntl.h>
#include <unistd.h>

#include "doctest.h"
#include "utils/scoped_pipe.hpp"

TEST_CASE("ClosesDescriptorOnDestruction") {
    int pipe_fd[2];
    REQUIRE(pipe(pipe_fd) == 0);

    int read_fd = pipe_fd[0];
    int write_fd = pipe_fd[1];

    {
        ScopedPipe sfd{read_fd, write_fd};
        CHECK(sfd.GetRawReadFD() == read_fd);
        CHECK(sfd.GetRawWriteFD() == write_fd);
    }

    CHECK(fcntl(read_fd, F_GETFD) == -1);
    CHECK(errno == EBADF);

    CHECK(fcntl(write_fd, F_GETFD) == -1);
    CHECK(errno == EBADF);
}

TEST_CASE("MoveSemanticsWork") {
    int pipe_fd[2];

    REQUIRE(pipe(pipe_fd) == 0);

    int read_fd = pipe_fd[0];
    int write_fd = pipe_fd[1];

    {
        ScopedPipe fd_1{read_fd, write_fd};
        ScopedPipe fd_2{std::move(fd_1)};

        CHECK(fd_1.GetRawReadFD() == -1);
        CHECK(fd_1.GetRawWriteFD() == -1);

        CHECK(fd_2.GetRawReadFD() == read_fd);
        CHECK(fd_2.GetRawWriteFD() == write_fd);

        CHECK(fcntl(read_fd, F_GETFD) != -1);
        CHECK(errno != EBADF);

        CHECK(fcntl(write_fd, F_GETFD) != -1);
        CHECK(errno != EBADF);
    }

    CHECK(fcntl(read_fd, F_GETFD) == -1);
    CHECK(errno == EBADF);

    CHECK(fcntl(write_fd, F_GETFD) == -1);
    CHECK(errno == EBADF);
}

TEST_CASE("MoveSemanticsByFakeOperatorWork") {
    int pipe_fd[2];

    REQUIRE(pipe(pipe_fd) == 0);

    int read_fd = pipe_fd[0];
    int write_fd = pipe_fd[1];

    {
        ScopedPipe fd_2 = ScopedPipe{read_fd, write_fd};

        CHECK(fcntl(read_fd, F_GETFD) != -1);
        CHECK(errno != EBADF);

        CHECK(fd_2.GetRawReadFD() == read_fd);
        CHECK(fd_2.GetRawWriteFD() == write_fd);

        CHECK(fcntl(read_fd, F_GETFD) != -1);
        CHECK(errno != EBADF);

        CHECK(fcntl(write_fd, F_GETFD) != -1);
        CHECK(errno != EBADF);
    }

    CHECK(fcntl(read_fd, F_GETFD) == -1);
    CHECK(errno == EBADF);

    CHECK(fcntl(write_fd, F_GETFD) == -1);
    CHECK(errno == EBADF);
}

TEST_CASE("MoveAssignmentOperatorWorks") {
    int pipe_fd[2];

    REQUIRE(pipe(pipe_fd) == 0);

    int read_fd = pipe_fd[0];
    int write_fd = pipe_fd[1];

    {
        ScopedPipe fd_1{read_fd, write_fd};
        ScopedPipe fd_2{-1, -1};

        fd_2 = std::move(fd_1);

        CHECK(fd_1.GetRawReadFD() == -1);
        CHECK(fd_1.GetRawWriteFD() == -1);

        CHECK(fd_2.GetRawReadFD() == read_fd);
        CHECK(fd_2.GetRawWriteFD() == write_fd);

        CHECK(fcntl(read_fd, F_GETFD) != -1);
        CHECK(errno != EBADF);

        CHECK(fcntl(write_fd, F_GETFD) != -1);
        CHECK(errno != EBADF);
    }

    CHECK(fcntl(read_fd, F_GETFD) == -1);
    CHECK(errno == EBADF);

    CHECK(fcntl(write_fd, F_GETFD) == -1);
    CHECK(errno == EBADF);
}