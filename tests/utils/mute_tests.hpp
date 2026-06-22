#pragma once

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

struct MuteSTDERR {
    int old_stderr;
    FILE* dev_null;

    MuteSTDERR() {
        old_stderr = dup(STDERR_FILENO);
        dev_null = fopen("/dev/null", "w");
        dup2(fileno(dev_null), STDERR_FILENO);
    }

    ~MuteSTDERR() {
        fflush(stderr);
        dup2(old_stderr, STDERR_FILENO);
        close(old_stderr);
        fclose(dev_null);
    }
};

struct MuteAllSTD {
    int old_stdout;
    int old_stderr;
    int dev_null;

    MuteAllSTD() {
        old_stdout = dup(STDOUT_FILENO);
        old_stderr = dup(STDERR_FILENO);

        dev_null = open("/dev/null", O_WRONLY);

        dup2(dev_null, STDOUT_FILENO);
        dup2(dev_null, STDERR_FILENO);
    }

    ~MuteAllSTD() {
        fflush(stdout);
        fflush(stderr);

        dup2(old_stdout, STDOUT_FILENO);
        dup2(old_stderr, STDERR_FILENO);

        close(old_stdout);
        close(old_stderr);
        close(dev_null);
    }
};