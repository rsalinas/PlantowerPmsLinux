#pragma once

#include <wait.h>
#include <csignal>

class Process {
public:
    Process(int pid) : pid_(pid) {
    }
    ~Process();

    static Process runCommand(int childFd,  const char** args);
    pid_t pid() const {
        return pid_;
    }
    int wait();
    int signal(int signo);

private:
    pid_t pid_ = 0;
};
