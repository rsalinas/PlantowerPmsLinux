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

volatile static bool sRunning = true;

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

void terminationHandler(int signum) {
    if (sRunning) {
        clog << "Shutdown requested" << endl;
        sRunning = false;        
    } else {
        clog << "You seem impatient, bailing out" << endl;
        exit(1);
    }
}
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


    struct sigaction action;

    action.sa_handler = terminationHandler;
    sigemptyset (&action.sa_mask);
    action.sa_flags = 0;
    for (auto signo : { SIGINT, SIGTERM })
        sigaction(signo, &action, NULL);

    //    Pms::RunResponse rr;
    //    do {
    //        clog << "Setting passive mode, we will poll" << endl;
    //        pms.setActive(false);
    //        usleep(500*1000);

    //    } while (rr = pms.run(),
    //             clog << "type: " << rr.type << " control ==" << rr.control << endl,
    //             rr.type != Pms::RunResponse::RESPONSE_TYPE_CONTROL
    //             || rr.control != 57600);
    //    pms.pollMeasurementInPassiveMode();
    //    rr = pms.run();
    //    clog << "Type: " << rr.type << endl;
    //    return 0;
    pms.setRunning(true);
    shared_ptr<void> setNotRunningGuard(NULL, [&](void*) {
        clog << "Stopping device" << endl;
        pms.setRunning(false);
    });

    switch (configParser.captureMode) {
    case ALWAYS_ON:
    {
        clog << "always on" << endl;
        pms.setActive(true);
        Pms::RunResponse rr;
        while (rr = pms.run(), sRunning && rr.type == Pms::RunResponse::RESPONSE_TYPE_MEASUREMENT) {
            clog << "Getting more measurements" << endl;
        }
        return rr.type != Pms::RunResponse::RESPONSE_TYPE_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    case SINGLE_SHOT:
    {
        pms.setActive(false);
        clog << "Waiting for stabilization" << endl;
//        sleep(5);
        clog << "Stable" << endl;
        pms.pollMeasurementInPassiveMode();
        auto rr = pms.run();
        if (rr.type != Pms::RunResponse::RESPONSE_TYPE_MEASUREMENT) {
            clog << "Error getting sample" << endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
        break;
    }
    case TIMED:
        clog << "timed" << endl;
        pms.setActive(false);
        bool startStop = configParser.captureInterval_secs >= 5;
        sleep(3);
        Pms::RunResponse rr;
        while (sRunning) {
            pms.pollMeasurementInPassiveMode();
            while (rr = pms.run(), rr.type != Pms::RunResponse::RESPONSE_TYPE_MEASUREMENT) {
                clog << "****** got: " << rr.type << " " << rr.control << endl;
                clog << "Waiting for measurement" << endl;
            }
            if (startStop) {
                pms.setRunning(false);
                sleep(configParser.captureInterval_secs - 3);
                pms.setRunning(true);
                sleep(3);
            } else {
                sleep(configParser.captureInterval_secs);
            }
        }
        return EXIT_SUCCESS;
    }
}
