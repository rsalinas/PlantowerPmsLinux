#include "PmsMock.h"

#include <iostream>

using namespace std;

void CommandProcessor::byteArrived(unsigned char ch) {
    switch (state_) {
    case INITIAL:
        if (ch == 'B') {
            state_ = HEADER2;
            cksum_ = ch;
        }
        break;
    case HEADER2:
        switch (ch) {
           case 'M':
            state_ = CMD;
            cksum_ += ch;
            break;
        default:
            state_ = INITIAL;
        }
        break;
    case CMD:
        cmd_ = ch;
        cksum_ += ch;
        state_ = DATAH;
        break;
    case DATAH:
        data_ = ch<<8;
        cksum_ += ch;
        state_ = DATAL;
        break;
    case DATAL:
        data_ |= ch;
        cksum_ += ch;
        state_ = CKSUM0;
        break;
    case CKSUM0:
        state_=CKSUM1;
        expectedCksum_ = ch << 8;
        break;
    case CKSUM1:
        expectedCksum_ |= ch;
        if (cksum_ == expectedCksum_) {
            clog << "Cksum OK" << endl;
            cb_(cmd_, data_);
        } else {
            clog << "bad cksum: " << cksum_ << " vs " << expectedCksum_ << endl;
        }
        break;
    }

}

void PmsMock::run() {
    volatile bool running = true;
    std::thread timerThread{[this, &running] () {
            while (running) {

                fd_set rfds;
                FD_ZERO(&rfds);
                FD_SET(0, &rfds);

                struct timeval tv;
                tv.tv_sec = 5;
                tv.tv_usec = 0;

                auto retval = select(1, &rfds, NULL, NULL, &tv);
                switch (retval) {
                case -1:
                    perror("select()");
                    exit(1);
                case 0:
                    // timeout
                    clog << "Every second" << endl;
                    break;
                default:
                    if (FD_ISSET(fd_, &rfds)) {

                    }
                }

            }
        }};
    running = false;
    timerThread.join();
}

void PmsMock::processCommand(unsigned char cmd, unsigned short data) {

}
