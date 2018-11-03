#include "Pms.h"
#include "GoogleDocs.h"
#include <iostream>
#include <cstring>
#include <functional>

#include "CliParser.h"
#include "Properties.h"
#include "Mqtt.h"

#include <unistd.h>
#include <thread>

using namespace std;

static GoogleDocsUploaderBackend gdub;
static MqttUploaderBackend mqttub;

enum RunningMode {
    MODE_NONE, MODE_RUNNING, MODE_SLEEPING
};

enum CaptureMode {
    ALWAYS_ON, SINGLE_SHOT, TIMED
};

struct ConfigParser : public ConfigParserBase {
    ConfigParser() : ConfigParserBase(
    {
    {{"-h", "-help"}, nullptr, "Show help", [this](const char**&) -> bool {
        showHelp();
        return false;
    }},
    {{"-d", "--device"}, "device", "Set device", [this](const char**&ptr) -> bool {
        device = *++ptr;
        return true;
    }},
    {{"-c", "--config"}, "configfile", "Set config file", [this](const char**&ptr) -> bool {
        configFile= *++ptr;
        return true;
    }},
    {{"-1", "--singleshot"}, nullptr, "Set single shot mode - get 1 sample and exit", [this](const char**&) -> bool {
        captureMode = SINGLE_SHOT;
        return true;
    }},
    {{"-t", "--timed"}, "seconds", "Set timed mode", [this](const char**&) -> bool {
        captureMode = TIMED;
        return true;
    }},
    {{"-v", "--verbose"}, nullptr, "Set high verbosity", [this](const char**&) -> bool {
        verbose = true;
        clog << "Setting verbose" << endl;
        return true;
    }},
    {{"-r", "--running"}, nullptr, "Set high verbosity", [this](const char**&) -> bool {
        clog << "Set running" << endl;
        runningMode = MODE_RUNNING;
        return true;
    }},
    {{"-s", "--sleeping"}, nullptr, "Set high verbosity", [this](const char**&) -> bool {
        clog << "Set sleeping" << endl;
        runningMode = MODE_SLEEPING;
        return true;
    }},
}) {
    }

    void dump() {
        clog << "Device: " << device << endl;
        clog << "Configfile: " << configFile << endl;

    }
    bool applyFromProperties(const Properties& props) {
        if (device.empty())
            device = props.getString("port", "/dev/ttyUSB0");
        return true;
    }

    std::string configFile{"pms.conf"};
    std::string device;
    CaptureMode captureMode = ALWAYS_ON;
    bool verbose = false;
    RunningMode runningMode = MODE_NONE;
};

int main(int argc, const char** argv)
{
    std::vector<Backend*> backends = { &gdub, &mqttub };
    ConfigParser configParser;
    if (!configParser.parse(argv)) {
        return EXIT_SUCCESS;
    }

    Properties props{configParser.configFile};
    configParser.applyFromProperties(props);
    props.dump();
    for (auto backend : backends)
        backend->initialize(props);
    Pms pms(configParser.device.c_str());
    for (auto backend : backends)
        if (*backend)
            backend->registerCallback(pms);
    if (configParser.runningMode == MODE_RUNNING) {
        pms.setRunning(true);
    } else if (configParser.runningMode == MODE_SLEEPING) {
        pms.setRunning(false);
    }
    pms.setRunning(true);
    std::thread th{[&]() {
            auto ret = pms.run(configParser.captureMode == SINGLE_SHOT);
            clog << "pms.run ended with " << ret << endl;
                   }};
    sleep(3);
    while (!pms.sleepConfirmed_) {
        clog << "Issuing sleep ..." << endl;
        pms.setRunning(false);
        sleep(1);
    }
    clog << "Sleep confirmed" << endl;
    th.join();
    //    pms.setRunning(false);
    //return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
