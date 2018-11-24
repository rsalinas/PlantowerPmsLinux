#include "Pms.h"

#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <array>

using namespace std;

std::vector<Backend*> Registry::backends_;

Registry::Registry(Backend& backend)  {
    clog << "Registering backend: " << backend.name() << endl;
    backends_.push_back(&backend);
}

std::string bufferToHex(const unsigned char* buffer, size_t length) {
    std::stringstream stream;
    stream << std::hex  << std::setfill ('0') << std::setw(sizeof(char)*2);
    for (size_t i =0; i < length; ++i) {
        stream << (unsigned int) buffer[i] << " ";
    }
    return stream.str();
}

bool Pms::sendCommand(unsigned char cmd, unsigned  char datah, unsigned  char datal) {
    std::array<unsigned char, 7> command = {0x42, 0x4d, cmd, datah, datal, 0, 0};
    short cksum = 0;
    for (auto ch : command)
        cksum += ch;
    command[5] = cksum >> 8 & 0xff;
    command[6] = cksum & 0xff;
    //    clog << "Issuing command: " << bufferToHex(command.cbegin(), command.size()) << endl;
    return write(fd_, command.begin(), command.size()) == sizeof command;
}

bool Pms::setActive(bool active) {
    clog << __FUNCTION__ << " " << int(active) << endl;
    return sendCommand(0xe1, 0, active ? 1 : 0);
}

bool Pms::setRunning(bool running) {
    clog << __FUNCTION__ << ": " << int(running) << endl;
    return sendCommand(0xe4, 0, running ? 1 : 0);
}

bool Pms::pollMeasurementInPassiveMode() {
    clog << __FUNCTION__ << endl;
    return sendCommand( 0xe2, 0, 0);
}

PollResult Pms::pollReadByte(char& ch, int timeout)
{
    fd_set rfds, efds;
    FD_ZERO(&rfds);
    FD_ZERO(&efds);
    FD_SET(fd_, &rfds);

    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = timeout % 1000 * 1000;

retry:
    switch (select(fd_ + 1, &rfds, NULL, &efds, &tv)) {
    case -1:
        if (errno == EINTR) {
            if (running_) {
                clog << "interrupted while running, retrying" << endl;
                goto retry;
            } else {
                return POLLRESULT_INTERRUPTED;
            }
        }
        perror("select()");
        return POLLRESULT_ERROR;
    case 0: //timeout
        clog << "Timeout after " << timeout << " ms" << endl;
        return POLLRESULT_TIMEOUT;
    default:
        if (FD_ISSET(fd_, &efds)) {
            clog << "Error bit is set" << endl;
            return POLLRESULT_ERROR;
        }
        if (!FD_ISSET(fd_, &rfds)) {
            clog << "FD has nothing" << endl;
            return POLLRESULT_EOF;
        } else {
            switch (read(fd_, &ch, 1)) {
            case 0:
                return POLLRESULT_EOF;
            case 1:
                return POLLRESULT_DATA;
            default:
                clog << "read said -1" << endl;
                return POLLRESULT_ERROR;
            }
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
    case POLLRESULT_INTERRUPTED:
        clog << "Interrupted by user" << endl;
        errorMessage = "closed";
        break;
    case POLLRESULT_DATA:
        //impossible
        break;
    }
    notifyError(errorMessage);

}
void Pms::processMeasurement(unsigned short* data) {
    static unsigned short lastone[12];

    if (memcmp(data, lastone, sizeof(lastone)) == 0) {
        // Data didn't change;
        return;
    }
    memcpy(lastone, data, sizeof lastone);

    for (auto i = 0u ; i < items.size(); i++) {
        clog << items[i].name << "=" << swapShort(data[i]) << ' ';
    }
    clog << endl;
    for (auto listener : listeners_)
        listener->update(data, items);
}

bool Pms::run(CaptureMode captureMode, int captureInterval_secs ) {
    setRunning(true);
    shared_ptr<void> setNotRunningGuard(NULL, [&](void*) {
        clog << "Stopping device" << endl;
        setRunning(false);
    });

    switch (captureMode) {
    case ALWAYS_ON:
    {
        clog << "always on" << endl;
        setActive(true);
        Pms::RunResponse rr;
        while (rr = run(), running_ && rr.type == Pms::RunResponse::RESPONSE_TYPE_MEASUREMENT) {
            clog << "Getting more measurements" << endl;
        }
        clog << "rr.type == " << rr.type << endl;
        return rr.type != Pms::RunResponse::RESPONSE_TYPE_ERROR;
    }
    case SINGLE_SHOT:
    {
        setActive(false);
        clog << "Waiting for stabilization" << endl;
        //        sleep(5);
        clog << "Stable" << endl;
        pollMeasurementInPassiveMode();
        auto rr = run();
        if (rr.type != Pms::RunResponse::RESPONSE_TYPE_MEASUREMENT) {
            clog << "Error getting sample" << endl;
            return false;
        }
        return true;
    }
    case TIMED:
        clog << "timed" << endl;
        setActive(false);
        setRunning(true);
        bool startStop = captureInterval_secs >= 5;
        sleep(3);
        Pms::RunResponse rr;
        while (running_) {
            pollMeasurementInPassiveMode();
            while (rr = run(), rr.type != Pms::RunResponse::RESPONSE_TYPE_MEASUREMENT) {
                if (!running_)
                    return true;
                clog << "****** got: " << rr.type << " " << rr.control
                     <<  " while waiting for measurement" << endl;
            }
            if (startStop) {
                clog << "START-STOP" << endl;
                setRunning(false);
                if (rr = run(), rr.type != Pms::RunResponse::RESPONSE_TYPE_CONTROL ||
                        rr.control != 58368) {
                    clog << "strange, our sleep command was not confirmed" << endl;
                } else {
                    clog << "good, sleep was confirmed" << endl;
                }
                sleep(captureInterval_secs - 3);
                setRunning(true);
                sleep(3);
            } else {
                sleep(captureInterval_secs);
            }
        }
        return true;
    }
    return false;
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

Pms::RunResponse Pms::run() {
    short len_ = 0;
    unsigned char ecksum_ = 0, cksum = 0;
    Measurement payload;
    unsigned short hostdata = 0;
    enum PacketType {
        TYPE_NONE, TYPE_MEASUREMENT, TYPE_CONTROL
    } packetType = TYPE_NONE;
    char b;
    PollResult result;
    while (result = pollReadByte(b, 5000), result == POLLRESULT_DATA) {
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
                    return { Pms::RunResponse::RESPONSE_TYPE_ERROR };
                    //setState(HEADER1);
                    break;
                }
                len_ = b << 8;
            } else {
                len_ |= b;
                switch (len_) {
                case 28:
                    setState(MEASUREMENT);
                    packetType = TYPE_MEASUREMENT;
                    break;
                case 4:
                    setState(CONTROLDATA);
                    packetType= TYPE_CONTROL;
                    break;
                default:
                    clog << "Wrong length received, corruption" << endl;
                    return {Pms::RunResponse::RESPONSE_TYPE_ERROR};
                }
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
                setState(HEADER1);
                if (ecksum_ == cksum)  {
                    if (packetType == TYPE_MEASUREMENT) {
                        processMeasurement(reinterpret_cast<unsigned short*>(payload));
                        return { Pms::RunResponse::RESPONSE_TYPE_MEASUREMENT };
                    } else {
                        processControl(hostdata);
                        return { Pms::RunResponse::RESPONSE_TYPE_CONTROL, {}, hostdata };
                    }
                } else {
                    clog << "Bad checksum: " << int(ecksum_) << " vs " << int(cksum) << endl;
                    return { Pms::RunResponse::RESPONSE_TYPE_ERROR };
                }
            }
            break;

        }
    }
    notifyError(result);
    return { Pms::RunResponse::RESPONSE_TYPE_ERROR };
}

void Pms::processControl(unsigned short data) {
    switch (data)  {
    case 58368:
        clog << "Sleeping confirmed" << endl;
        break;
    case 57600:
        clog << "Passive mode confirmed" << endl;
        break;
    case 57601:
        clog << "Active mode confirmed" << endl;
        break;
    default:
        clog << "Unknown control data: " << data << endl;
    }
}

Pms::~Pms() {
    clog << __FUNCTION__ << endl;
    if (close(fd_) < 0) {
        auto errorString = strerror(errno);
        clog << "Warning: error closing serial port: " << errorString << endl;
    }
}

void Pms::stop() {
    clog << "pms stopped" << endl;
    running_ = false;
}
