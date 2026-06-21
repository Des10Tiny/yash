#include "doctest.h"
#include "utils/unique_fd.hpp"

#include <cstdio>
#include <fcntl.h>
#include <filesystem>
#include <unistd.h>

class UniqueFDTest {
protected:
    bool IsDescriptorValid(int fd) {
        if (fd < 0) {
            return false;
        }
        return fcntl(fd, F_GETFD) != -1;
    }
};

TEST_CASE_FIXTURE(UniqueFDTest, "ClosesDescriptorOnDestruction") {
    int raw_fd = open("/dev/null", O_RDONLY);
    REQUIRE(raw_fd > 0);
    {
        UniqueFD ufd{raw_fd};
        CHECK(ufd.GetRawFD() == raw_fd);
        CHECK(IsDescriptorValid(raw_fd));
    }

    CHECK_FALSE(IsDescriptorValid(raw_fd));
    CHECK(errno == EBADF);
}

TEST_CASE_FIXTURE(UniqueFDTest, "MoveConstructorTransfersOwnership") {
    int raw_fd = open("/dev/null", O_RDONLY);
    UniqueFD ufd1{raw_fd};

    UniqueFD ufd2{std::move(ufd1)};

    CHECK(ufd1.GetRawFD() == -1);
    CHECK(ufd2.GetRawFD() == raw_fd);
    CHECK(IsDescriptorValid(raw_fd));
}

TEST_CASE_FIXTURE(UniqueFDTest, "MoveAssignmentClosesCurrentAndTakesNew") {
    int fd1 = open("/dev/null", O_RDONLY);
    int fd2 = open("/dev/null", O_RDONLY);

    {
        UniqueFD ufd1{fd1};
        UniqueFD ufd2{fd2};

        ufd2 = std::move(ufd1);

        CHECK_FALSE(IsDescriptorValid(fd2));
        CHECK(ufd2.GetRawFD() == fd1);
        CHECK(ufd1.GetRawFD() == -1);
    }

    CHECK_FALSE(IsDescriptorValid(fd1));
}

TEST_CASE_FIXTURE(UniqueFDTest, "SelfAssignmentDoesNothing") {
    int raw_fd = open("/dev/null", O_RDONLY);
    UniqueFD ufd{raw_fd};

    ufd = std::move(ufd);

    CHECK(ufd.GetRawFD() == raw_fd);
    CHECK(IsDescriptorValid(raw_fd));
}

TEST_CASE_FIXTURE(UniqueFDTest, "CreateNewFDPensFileCorrectly") {
    UniqueFD ufd;

    REQUIRE_NOTHROW((void)ufd.CreateNewFD("/dev/null", O_RDONLY));

    int fd = ufd.GetRawFD();
    CHECK(fd > 0);
    CHECK(IsDescriptorValid(fd));
}

TEST_CASE_FIXTURE(UniqueFDTest, "CreateNewFDClosesPreviousDescriptor") {
    int first_fd = open("/dev/null", O_RDONLY);
    REQUIRE(first_fd > 0);

    UniqueFD ufd{first_fd};

    int dummy_fd = open("/dev/null", O_RDONLY);

    (void)ufd.CreateNewFD("/dev/null", O_RDONLY);
    close(dummy_fd);
}

TEST_CASE_FIXTURE(UniqueFDTest, "CreateNewFDNoThrowsOnInvalidFile") {
    UniqueFD ufd;

    bool status = true;
    CHECK_NOTHROW(status = ufd.CreateNewFD("/non_existent_file_12345", O_RDONLY));

    CHECK(status == false);
    CHECK(ufd.GetRawFD() == -1);
}

TEST_CASE_FIXTURE(UniqueFDTest, "CloseFDResetsState") {
    int raw_fd = open("/dev/null", O_RDONLY);
    UniqueFD ufd{raw_fd};

    ufd.CloseFD();

    CHECK(ufd.GetRawFD() == -1);
    CHECK_FALSE(IsDescriptorValid(raw_fd));
}

TEST_CASE_FIXTURE(UniqueFDTest, "MultipleMovesWork") {
    UniqueFD ufd1{open("/dev/null", O_RDONLY)};
    int raw_fd = ufd1.GetRawFD();

    UniqueFD ufd2 = std::move(ufd1);
    UniqueFD ufd3 = std::move(ufd2);

    CHECK(ufd1.GetRawFD() == -1);
    CHECK(ufd2.GetRawFD() == -1);
    CHECK(ufd3.GetRawFD() == raw_fd);
    CHECK(IsDescriptorValid(raw_fd));
}

TEST_CASE_FIXTURE(UniqueFDTest, "DestructorClosesFileDescriptor") {
    int raw_fd_copy = -1;
    const char* test_file = "/tmp/yash_fd_test.txt";

    std::ofstream f(test_file);
    f << "test";
    f.close();

    {
        UniqueFD ufd;
        bool status = ufd.CreateNewFD(test_file, O_RDONLY);
        REQUIRE(status);

        raw_fd_copy = ufd.GetRawFD();
        REQUIRE(raw_fd_copy > 2);

        char buf[1];
        CHECK(read(raw_fd_copy, buf, 1) == 1);
    }

    char buf[1];
    ssize_t read_result = read(raw_fd_copy, buf, 1);

    CHECK(read_result == -1);
    CHECK(errno == EBADF);

    std::filesystem::remove(test_file);
}