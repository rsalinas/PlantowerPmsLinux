#include "Process.h"
#include <iostream>

#include <unistd.h>


using namespace std;

Process Process::runCommand(int childFd,  const char* * args) {
    int pid = fork();
    switch (pid) {
    case -1:
        perror("fork. Serious");
        exit(1);
    case 0:
        //child
        for (int fd = 3; fd < 1024; ++fd) {
            if (fd != childFd)
                if (!close(fd)) {
                    clog << "fd " << fd << " has been closed in the child" << endl;
                }
        }
        execvp(args[0], const_cast<char*const *>(args));
        perror("execvp");
        exit(1);
    default:
        close(childFd);
    }
    return Process{pid};
}
Process::~Process() {
    if (pid_)
        clog << "Forgetting child. " << pid_ << endl;
}

int Process::wait() {
    int status;
    if (waitpid(pid_, &status, 0) < 0) {
        perror("Error in waitpid()");
        return -1;
    }
    pid_ = 0;
    return WEXITSTATUS(status);
}

int Process::signal(int signo) {
    return kill(pid_, signo);
}
