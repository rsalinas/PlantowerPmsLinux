#include "Pms.h"

#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>

using namespace std;

PollResult pollReadByte(char& ch, int fd, int timeout)
{
    fd_set rfds, efds;
    FD_ZERO(&rfds);
    FD_ZERO(&efds);
    FD_SET(fd, &rfds);

    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = timeout % 1000 * 1000;

    switch (select(fd + 1, &rfds, NULL, &efds, &tv)) {
    case -1:
        perror("select()");
        return POLLRESULT_ERROR;
    case 0: //timeout
        clog << "Timeout after " << timeout << " ms" << endl;
        return POLLRESULT_TIMEOUT;
    default:
        if (FD_ISSET(fd, &efds)) {
            clog << "Error bit is set" << endl;
            return POLLRESULT_ERROR;
        }
        if (!FD_ISSET(fd, &rfds)) {
            clog << "FD has nothing" << endl;
            return POLLRESULT_EOF;
        }
        switch (read(fd, &ch, 1)) {
        case 0:
            return POLLRESULT_EOF;
        case 1:
            return POLLRESULT_DATA;
        default:
            return POLLRESULT_ERROR;
        }
    }
}



const std::vector<Item> items= {{"P_CF_PM10", "CF=1, PM1.0", "μg/m3"},
                                {"P_CF_PM25", "CF=1, PM2.5", "μg/m3"},
                                {"P_CF_PM100", "CF=1, PM10", "μg/m3"},
                                {"P_C_PM10", "PM1.02", "μg/m3"},
                                {"P_C_PM25", "PM2.5", "μg/m3"},
                                {"P_C_PM100", "PM10", "μg/m3"},
                                {"P_C_03", "0.1L, d>0.3μm", ""},
                                {"P_C_05", "0.1L, d>0.5μm", ""},
                                {"P_C_10", "0.1L, d>1μm", ""},
                                {"P_C_25", "0.1L, d>2.5μm", ""},
                                {"P_C_50", "0.1L, d>5.0μm", ""},
                                {"P_C_100", "0.1L, d>10μm", ""},
                               };



void Pms::notifyError(const char* message) {
    for (auto& listener : listeners_)
        listener->onError(message);
}


void Pms::notifyError(PollResult result) {
    const char* errorMessage = nullptr;
    switch (result) {
    case POLLRESULT_EOF:
        clog << "Port was closed" << endl;
        errorMessage = "portclosed";
        break;
    case POLLRESULT_ERROR:
        clog << "Port error" << endl;
        errorMessage = "porterror";
        break;
    case POLLRESULT_TIMEOUT:
        clog << "Timeout: device was silent too long";
        errorMessage = "unresponsive";
        break;
    }
    notifyError(errorMessage);

}
void Pms::processMeasurement(unsigned short* data) {
    static unsigned short lastone[12];

    if (memcmp(data, lastone, sizeof(lastone))== 0) {
        //        clog << "Data didn't change";
        return;
    }
    memcpy(lastone, data, sizeof lastone);

    for (int i = 0 ; i < items.size(); i++) {
        clog << items[i].name << "=" << swapShort(data[i]) << ' ';
    }
    clog << endl;
    for (auto listener : listeners_)
        listener->update(data, items);
}

Pms::Pms(const char* port) : fd_(open(port, O_RDWR| O_NOCTTY)) {
    if (fd_ < 0) {
        perror("Cannot open serial port");
        exit(1);
    }

    //    fcntl(fd, F_SETFL, O_ASYNC|O_NONBLOCK|fcntl(fd, F_GETFL));

    struct termios tty;
    struct termios tty_old;
    memset(&tty, 0, sizeof tty);

    if ( tcgetattr ( fd_, &tty ) != 0 ) {
        std::cout << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
    }

    /* Save old tty parameters */
    tty_old = tty;

    /* Set Baud Rate */
    cfsetospeed (&tty, (speed_t)B9600);
    cfsetispeed (&tty, (speed_t)B9600);

    /* Setting other Port Stuff */
    tty.c_cflag     &=  ~PARENB;            // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;

    tty.c_cflag     &=  ~CRTSCTS ;           // no flow control
    tty.c_cflag     &= ~ICANON; /* Set non-canonical mode */
    tty.c_cc[VMIN]   = 1; // 1;                  // read doesn't block
    tty.c_cc[VTIME]  = 0; // 5;                  // 0.5 seconds read timeout
    tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

    cfmakeraw(&tty);

    tcflush(fd_, TCIFLUSH );
    if ( tcsetattr ( fd_, TCSANOW, &tty ) != 0) {
        std::cout << "Error " << errno << " from tcsetattr" << std::endl;
    }
    //    tcflush( fd, TCIFLUSH );
}

Pms::~Pms() {
}

bool Pms::run(bool runOnce) {
    char b;
    short len_;
    unsigned char ecksum_ = 0, cksum = 0;
    char payload[13*sizeof (short)];
    short hostdata;
    enum PacketType {
        TYPE_MEASUREMENT, TYPE_CONTROL
    } packetType;
    PollResult result;
    while (result = pollReadByte(b, fd_, 3000), result == POLLRESULT_DATA) {
        switch (state_) {
        case HEADER1:
            if (b == 'B') {
                setState(HEADER2);
                cksum = b;
            }
            break;
        case HEADER2:
            if (b == 'M') {
                setState(LENGTH);
                cksum += b;
            } else {
                setState(HEADER1);
            }
            break;
        case LENGTH:
            cksum += b;
            if (statePos_++ == 0) {
                if (b!= 0) {
                    clog << "High byte of length is not null" << endl;
                    setState(HEADER1);
                    break;
                }
                len_ = b << 8;
            } else {
                len_ |= b;
                setState(len_ == 28 ? MEASUREMENT : CONTROLDATA);
                packetType = len_ == 28 ? TYPE_MEASUREMENT : TYPE_CONTROL;
            }
            break;
        case MEASUREMENT:
            cksum += b;
            payload[statePos_] = b;
            statePos_++;
            if (statePos_ == len_ - 2) {
                setState(CKSUM);
            }
            break;
        case CONTROLDATA:
            cksum += b;
            if (statePos_++ == 0) {
                hostdata = b << 8;
            } else {
                hostdata |= b;

                setState(CKSUM);
            }
            break;
        case CKSUM:
            if (statePos_++ == 0) {
                ecksum_ = b << 8;
            } else {
                ecksum_ |= b;
                if (ecksum_ == cksum)  {
                    if (packetType == TYPE_MEASUREMENT) {
                        processMeasurement(reinterpret_cast<unsigned short*>(payload));
                        if (runOnce)
                            return true;
                    } else {
                        processControl(hostdata);
                    }
                } else {
                    clog << "Bad checksum: " << int(ecksum_) << " vs " << int(cksum) << endl;
                }
                setState(HEADER1);
            }
            break;

        }
    }
    notifyError(result);
    return false;
}

void Pms::processControl(unsigned short data) {
    clog << "Control: " << data << endl;
}