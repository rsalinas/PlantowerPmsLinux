#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <thread>

class Properties;

enum CaptureMode {
    ALWAYS_ON, SINGLE_SHOT, TIMED
};

struct Item {
    Item(const std::string& name, const std::string& desc, const std::string& unit) :
        name(name), desc(desc), unit(unit) {
    }

    std::string name;
    std::string desc;
    std::string unit;
};

inline unsigned short swapShort(unsigned short i) {
    return (i&&0xFF) << 8 | (i >> 8);
}

class PmsMeasurementCallback {
public:
    virtual void update(const unsigned short* data, const std::vector<Item>& metadata) = 0;
    virtual void onError(const char* message) = 0;
};

enum PollResult {
    POLLRESULT_DATA, POLLRESULT_TIMEOUT, POLLRESULT_EOF, POLLRESULT_ERROR, POLLRESULT_INTERRUPTED
};


class Pms {
public:
    const int stabilizationTime_ms = 30*1000;
    Pms(const char* port);
    ~Pms();

    enum State {
        HEADER1, HEADER2, LENGTH, MEASUREMENT, CONTROLDATA, CKSUM
    };
    typedef char Measurement[13*sizeof (short)];

    struct RunResponse {
        enum Type {
            RESPONSE_TYPE_NONE, RESPONSE_TYPE_ERROR, RESPONSE_TYPE_MEASUREMENT,RESPONSE_TYPE_CONTROL
        } type;
        Measurement measurement;
        unsigned short control;
    };
    RunResponse run();

    void addListener(PmsMeasurementCallback& callback) {
        listeners_.push_back(&callback);
    }
    void removeListener(PmsMeasurementCallback& callback) {
        auto pos = std::find(listeners_.begin(), listeners_.end(), &callback);
        if (pos != listeners_.end())
            listeners_.erase(pos);
    }
    void setState (State newState) {
        state_ = newState;
        statePos_ = 0;
    }

    bool sendCommand(unsigned char cmd, unsigned char datahigh, unsigned char datalow);
    bool setActive(bool active);
    bool setRunning(bool running);
    bool pollMeasurementInPassiveMode();
    bool run(CaptureMode captureMode, int captureInterval_secs );
    bool isStopping() const {
        return !running_;
    }
    void stop() {
        running_ = false;
    }

private:
    PollResult pollReadByte(char& ch, int timeout);

    void processMeasurement(unsigned short* data);
    void notifyError(PollResult result);
    void notifyError(const char* message);
    void processControl(unsigned short data);
    const int fd_;
    State state_ = HEADER1;
    unsigned char statePos_ = 0;
    std::vector<PmsMeasurementCallback*> listeners_;
    volatile bool running_ = true;
};

class Backend {
public:
    virtual bool initialize(const Properties& props) = 0;
    virtual bool registerCallback(Pms& pms) = 0;
    virtual operator bool() const = 0;
};
