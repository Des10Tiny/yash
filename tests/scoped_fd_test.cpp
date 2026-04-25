#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>

#include "utils/scoped_fd.hpp"

TEST(ScopedFDTest, ClosesDescriptorOnDestruction) {
    int pipe_fd[2];
    ASSERT_EQ(pipe(pipe_fd), 0);

    int read_fd = pipe_fd[0];
    int write_fd = pipe_fd[1];

    {
        ScopedFD sfd{read_fd, write_fd};
        EXPECT_EQ(sfd.GetRawReadFD(), read_fd);
        EXPECT_EQ(sfd.GetRawWriteFD(), write_fd);
    }

    EXPECT_EQ(fcntl(read_fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);

    EXPECT_EQ(fcntl(write_fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
}

TEST(ScopedFDTest, MoveSemanticsWork) {
    int pipe_fd[2];

    ASSERT_EQ(pipe(pipe_fd), 0);

    int read_fd = pipe_fd[0];
    int write_fd = pipe_fd[1];

    {
        ScopedFD fd_1{read_fd, write_fd};
        ScopedFD fd_2{std::move(fd_1)};

        EXPECT_EQ(fd_1.GetRawReadFD(), -1);
        EXPECT_EQ(fd_1.GetRawWriteFD(), -1);

        EXPECT_EQ(fd_2.GetRawReadFD(), read_fd);
        EXPECT_EQ(fd_2.GetRawWriteFD(), write_fd);

        EXPECT_NE(fcntl(read_fd, F_GETFD), -1);
        EXPECT_NE(errno, EBADF);

        EXPECT_NE(fcntl(write_fd, F_GETFD), -1);
        EXPECT_NE(errno, EBADF);
    }

    EXPECT_EQ(fcntl(read_fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);

    EXPECT_EQ(fcntl(write_fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
}

TEST(ScopedFDTest, MoveSemanticsByFakeOperatorWork) {
    int pipe_fd[2];

    ASSERT_EQ(pipe(pipe_fd), 0);

    int read_fd = pipe_fd[0];
    int write_fd = pipe_fd[1];

    {
        ScopedFD fd_2 = ScopedFD{read_fd, write_fd};

        EXPECT_NE(fcntl(read_fd, F_GETFD), -1);
        EXPECT_NE(errno, EBADF);

        EXPECT_EQ(fd_2.GetRawReadFD(), read_fd);
        EXPECT_EQ(fd_2.GetRawWriteFD(), write_fd);

        EXPECT_NE(fcntl(read_fd, F_GETFD), -1);
        EXPECT_NE(errno, EBADF);

        EXPECT_NE(fcntl(write_fd, F_GETFD), -1);
        EXPECT_NE(errno, EBADF);
    }

    EXPECT_EQ(fcntl(read_fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);

    EXPECT_EQ(fcntl(write_fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
}

TEST(ScopedFDTest, MoveAssignmentOperatorWorks) {
    int pipe_fd[2];

    ASSERT_EQ(pipe(pipe_fd), 0);

    int read_fd = pipe_fd[0];
    int write_fd = pipe_fd[1];

    {
        ScopedFD fd_1{read_fd, write_fd};
        ScopedFD fd_2{-1, -1};

        fd_2 = std::move(fd_1);

        EXPECT_EQ(fd_1.GetRawReadFD(), -1);
        EXPECT_EQ(fd_1.GetRawWriteFD(), -1);

        EXPECT_EQ(fd_2.GetRawReadFD(), read_fd);
        EXPECT_EQ(fd_2.GetRawWriteFD(), write_fd);

        EXPECT_NE(fcntl(read_fd, F_GETFD), -1);
        EXPECT_NE(errno, EBADF);

        EXPECT_NE(fcntl(write_fd, F_GETFD), -1);
        EXPECT_NE(errno, EBADF);
    }

    EXPECT_EQ(fcntl(read_fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);

    EXPECT_EQ(fcntl(write_fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
}