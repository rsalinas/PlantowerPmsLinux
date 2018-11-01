#include "Pms.h"
#include "GoogleDocs.h"
#include <iostream>
#include <cstring>
#include <functional>

#include "CliParser.h"
#include "Properties.h"
#include "Mqtt.h"

using namespace std;

static GoogleDocsUploaderBackend gdub;
static MqttUploaderBackend mqttub;

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
        singleShot = true;
        return true;
    }},
    {{"-v", "--verbose"}, nullptr, "Set high verbosity", [this](const char**&) -> bool {
        verbose = true;
        clog << "Setting verbose" << endl;
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
    }
    std::string configFile{"pms.conf"};
    std::string device;
    bool singleShot = false;
    bool verbose = false;

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
    return pms.run(configParser.singleShot) ? EXIT_SUCCESS : EXIT_FAILURE;
}
