#pragma once
#include <thread>
#include <unistd.h>
#include <functional>

class CommandProcessor {
public:
    enum State {
        INITIAL, HEADER2, CMD, DATAH, DATAL, CKSUM0, CKSUM1
    } state_ = INITIAL;

    CommandProcessor(std::function<void(char, unsigned short)> cb) : cb_(std::move(cb)) {
    }
    void byteArrived(unsigned char ch);
private:
    std::function<void(char, unsigned short)> cb_;

    unsigned short cmd_ = 0;
    unsigned short cksum_ = 0;
    unsigned short expectedCksum_ = 0;
    unsigned short data_ = 0;
};


class PmsMock {
public:
    PmsMock(int fd) : commandProcessor_{std::bind(&PmsMock::processCommand, this, std::placeholders::_1, std::placeholders::_2)}, fd_(fd) {

    }
    void processCommand(unsigned char cmd, unsigned short data);
    void run();

    ~PmsMock() {
        close(fd_);
    }
private:
    CommandProcessor commandProcessor_;
    const int fd_;
    std::thread timerThread_;
};
