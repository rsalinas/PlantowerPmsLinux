#include "Pms.h"
#include "GoogleDocs.h"
#include <iostream>
#include <cstring>
#include "Properties.h"
#include "Mqtt.h"

using namespace std;

static GoogleDocsUploaderBackend gdub;
static MqttUploaderBackend mqttub;

struct ConfigParser  {
    ConfigParser(const char**argv) {
        parse(argv);
    }
    void parse(const char ** argv) {
        for (const char**ptr = argv+1; *ptr; ptr++) {
            if (!strcmp(*ptr, "-d") || !strcmp(*ptr, "--device")) {
                device = *++ptr;
            }
            if (!strcmp(*ptr, "-c") || !strcmp(*ptr, "--config")) {
                configFile= *++ptr;
            }
            if (!strcmp(*ptr, "-1") || !strcmp(*ptr, "--singleshot")) {
                singleShot = true;
            }
            if (!strcmp(*ptr, "-v") || !strcmp(*ptr, "--verbose")) {
                verbose = true;
            }
        }
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
    ConfigParser configParser{argv};
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
