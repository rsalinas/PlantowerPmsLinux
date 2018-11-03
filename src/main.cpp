#include "Pms.h"
#include "GoogleDocs.h"
#include <iostream>
#include <cstring>
#include <functional>

#include "CliParser.h"
#include "Properties.h"
#include "Mqtt.h"
#include <csignal>

#include <unistd.h>
#include <thread>

using namespace std;

static GoogleDocsUploaderBackend gdub;
static MqttUploaderBackend mqttub;

enum RunningMode {
    MODE_NONE, MODE_RUNNING, MODE_SLEEPING
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
    {{"-t", "--timed"}, "seconds", "Set timed mode", [this](const char**&ptr) -> bool {
        captureMode = TIMED;
        captureInterval_secs = atoi(*++ptr);
        if (captureInterval_secs <= 0) {
            clog << "Capture interval must be a positive number" << endl;
        }
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
    int captureInterval_secs = 0;
};

class SignalSetter {
public:
    SignalSetter(int signo, void (*handler)(int)) : signo_(signo) {
        struct sigaction action;
        action.sa_handler = handler;
        sigemptyset (&action.sa_mask);
        action.sa_flags = 0;
        sigaction(signo, &action, &oldAction);
    }
    ~SignalSetter() {
        sigaction(signo_, &oldAction, nullptr);
    }

private:
    const int signo_;
    struct sigaction oldAction;
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
    static Pms pms(configParser.device.c_str());

    for (auto backend : backends)
        if (*backend)
            backend->registerCallback(pms);
    if (configParser.runningMode == MODE_RUNNING) {
        pms.setRunning(true);
        return EXIT_SUCCESS;
    } else if (configParser.runningMode == MODE_SLEEPING) {
        pms.setRunning(false);
        return EXIT_SUCCESS;
    }

    auto terminationHandler = [](int signum) {
        if (!pms.isStopping()) {
            clog << "Shutdown requested" << endl;
            pms.stop();
        } else {
            clog << "You seem impatient, bailing out" << endl;
            exit(1);
        }
    };
    SignalSetter signalHandlerSIGINT{SIGINT, terminationHandler};
    SignalSetter signalHandlerSIGTERM{SIGTERM, terminationHandler};
    return pms.run(configParser.captureMode, configParser.captureInterval_secs) ? EXIT_SUCCESS : EXIT_FAILURE;
}
