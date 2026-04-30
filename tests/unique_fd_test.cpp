#include "utils/unique_fd.hpp"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

class UniqueFDTest : public ::testing::Test {
protected:
    bool IsDescriptorValid(int fd) {
        if (fd < 0) {
            return false;
        }
        return fcntl(fd, F_GETFD) != -1;
    }
};

TEST_F(UniqueFDTest, ClosesDescriptorOnDestruction) {
    int raw_fd = open("/dev/null", O_RDONLY);
    ASSERT_GT(raw_fd, 0);
    {
        UniqueFD ufd{raw_fd};
        EXPECT_EQ(ufd.GetRawFD(), raw_fd);
        EXPECT_TRUE(IsDescriptorValid(raw_fd));
    }

    EXPECT_FALSE(IsDescriptorValid(raw_fd));
    EXPECT_EQ(errno, EBADF);
}

TEST_F(UniqueFDTest, MoveConstructorTransfersOwnership) {
    int raw_fd = open("/dev/null", O_RDONLY);
    UniqueFD ufd1{raw_fd};

    UniqueFD ufd2{std::move(ufd1)};

    EXPECT_EQ(ufd1.GetRawFD(), -1);
    EXPECT_EQ(ufd2.GetRawFD(), raw_fd);
    EXPECT_TRUE(IsDescriptorValid(raw_fd));
}

TEST_F(UniqueFDTest, MoveAssignmentClosesCurrentAndTakesNew) {
    int fd1 = open("/dev/null", O_RDONLY);
    int fd2 = open("/dev/null", O_RDONLY);

    {
        UniqueFD ufd1{fd1};
        UniqueFD ufd2{fd2};

        ufd2 = std::move(ufd1);

        EXPECT_FALSE(IsDescriptorValid(fd2));
        EXPECT_EQ(ufd2.GetRawFD(), fd1);
        EXPECT_EQ(ufd1.GetRawFD(), -1);
    }

    EXPECT_FALSE(IsDescriptorValid(fd1));
}

TEST_F(UniqueFDTest, SelfAssignmentDoesNothing) {
    int raw_fd = open("/dev/null", O_RDONLY);
    UniqueFD ufd{raw_fd};

    ufd = std::move(ufd);

    EXPECT_EQ(ufd.GetRawFD(), raw_fd);
    EXPECT_TRUE(IsDescriptorValid(raw_fd));
}

TEST_F(UniqueFDTest, CreateNewFDPensFileCorrectly) {
    UniqueFD ufd;

    ASSERT_NO_THROW((void)ufd.CreateNewFD("/dev/null", O_RDONLY));

    int fd = ufd.GetRawFD();
    EXPECT_GT(fd, 0);
    EXPECT_TRUE(IsDescriptorValid(fd));
}

TEST_F(UniqueFDTest, CreateNewFDClosesPreviousDescriptor) {
    int first_fd = open("/dev/null", O_RDONLY);
    ASSERT_GT(first_fd, 0);

    UniqueFD ufd{first_fd};

    int dummy_fd = open("/dev/null", O_RDONLY);

    (void)ufd.CreateNewFD("/dev/null", O_RDONLY);
    close(dummy_fd);
}

TEST_F(UniqueFDTest, CreateNewFDNoThrowsOnInvalidFile) {
    UniqueFD ufd;

    bool status = true;
    EXPECT_NO_THROW(status = ufd.CreateNewFD("/non_existent_file_12345", O_RDONLY));

    EXPECT_EQ(status, false);
    EXPECT_EQ(ufd.GetRawFD(), -1);
}

TEST_F(UniqueFDTest, CloseFDResetsState) {
    int raw_fd = open("/dev/null", O_RDONLY);
    UniqueFD ufd{raw_fd};

    ufd.CloseFD();

    EXPECT_EQ(ufd.GetRawFD(), -1);
    EXPECT_FALSE(IsDescriptorValid(raw_fd));
}

TEST_F(UniqueFDTest, MultipleMovesWork) {
    UniqueFD ufd1{open("/dev/null", O_RDONLY)};
    int raw_fd = ufd1.GetRawFD();

    UniqueFD ufd2 = std::move(ufd1);
    UniqueFD ufd3 = std::move(ufd2);

    EXPECT_EQ(ufd1.GetRawFD(), -1);
    EXPECT_EQ(ufd2.GetRawFD(), -1);
    EXPECT_EQ(ufd3.GetRawFD(), raw_fd);
    EXPECT_TRUE(IsDescriptorValid(raw_fd));
}

TEST_F(UniqueFDTest, DestructorClosesFileDescriptor) {
    int raw_fd_copy = -1;
    const char* test_file = "/tmp/yash_fd_test.txt";

    std::ofstream f(test_file);
    f << "test";
    f.close();

    {
        UniqueFD ufd;
        bool status = ufd.CreateNewFD(test_file, O_RDONLY);
        ASSERT_TRUE(status);

        raw_fd_copy = ufd.GetRawFD();
        ASSERT_GT(raw_fd_copy, 2);

        char buf[1];
        EXPECT_EQ(read(raw_fd_copy, buf, 1), 1);
    }

    char buf[1];
    ssize_t read_result = read(raw_fd_copy, buf, 1);

    EXPECT_EQ(read_result, -1);
    EXPECT_EQ(errno, EBADF);

    std::filesystem::remove(test_file);
}