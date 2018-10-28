#include "Properties.h"

#include <fstream>
#include <iostream>

using namespace std;

const std::string Properties::NO_DEFAULT_VALUE;

Properties::Properties(const std::string& filename) {
    ifstream ifs;
    ifs.open(filename.c_str(), std::ifstream::in);
    while (ifs.good()) {
        std::string buffer;
        ifs >> buffer;
        if (buffer[0] == '#')
            continue;
        auto kvDelimiterPos = buffer.find('=');
        if (kvDelimiterPos != std::string::npos) {
            auto key = buffer.substr(0, kvDelimiterPos);
            auto value = buffer.substr(kvDelimiterPos+1);
            map_[key] = value;
            //
        }
    }
}

Properties::~Properties() {
}

std::string Properties::getString(const std::string& key,
                                  const std::string& defaultValue) const {
    auto it = map_.find(key);
    if (it != map_.end()) {
        return it->second;
    } else {
        if (&defaultValue != &NO_DEFAULT_VALUE) {
            return defaultValue;
        } else {
            throw KeyNotFound{};
        }
    }
}

int Properties::getInt(const std::string& key, int defaultValue) const {
    try {
        return atoi(getString(key).c_str());
    } catch (...) {
        return defaultValue;
    }
}

void Properties::dump() const {
    for (const auto& kv : map_) {
        clog << "Read: " << kv.first << " -> " << kv.second << endl;
    }
}


