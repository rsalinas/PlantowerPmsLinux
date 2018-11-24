#pragma once

#include <sys/socket.h>

class SocketPair {
public:
    SocketPair() : ok_(socketpair(PF_LOCAL, SOCK_STREAM, 0, fd_) == 0){
    }

    bool good() const {
        return ok_;
    }

    int fd0() const {
        return fd_[0];
    }
    int fd1() const {
        return fd_[1];
    }

private:
    const bool ok_;
    int fd_[2];

};
