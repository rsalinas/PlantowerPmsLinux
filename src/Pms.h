#pragma once

#include <string>
#include <vector>
#include <algorithm>

class Properties;

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
    POLLRESULT_DATA, POLLRESULT_TIMEOUT, POLLRESULT_EOF, POLLRESULT_ERROR
};


class Pms {
public:
    Pms(const char* port);
    ~Pms();
    bool run(bool runOnce = false);
    enum State {
        HEADER1, HEADER2, LENGTH, MEASUREMENT, CONTROLDATA, CKSUM
    };
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
private:
    void processMeasurement(unsigned short* data);
    void notifyError(PollResult result);
    void notifyError(const char* message);
    void processControl(unsigned short data);
    const int fd_;
    State state_ = HEADER1;
    unsigned char statePos_ = 0;
    std::vector<PmsMeasurementCallback*> listeners_;
};

class Backend {
public:
    virtual bool initialize(const Properties& props) = 0;
    virtual bool registerCallback(Pms& pms) = 0;
    virtual operator bool() const = 0;
};
