#pragma once

#include <memory>

#include "Pms.h"

class GoogleDocsUploader : public PmsMeasurementCallback {
public:
    typedef uint32_t google_entry_t;
    GoogleDocsUploader(const char* url, const std::vector<google_entry_t>& itemList);
    void update(const unsigned short* data, const std::vector<Item>& metadata) override ;
    void setInterval(int seconds) {
        intervalSeconds_ = seconds;
    }
    static std::vector<google_entry_t> splitOnToken(const std::string& s, char delimiter);
    void onError(const char* message) {
        // TODO
    }
private:
    std::string url_;
    const std::vector<google_entry_t> itemList_;

    int intervalSeconds_ = 60;
    time_t lastSuccessfulUpdate_ = 0;
};

class GoogleDocsUploaderBackend : public Backend {
public:
    bool initialize(const Properties& props) override;
    bool registerCallback(Pms& pms) override;
    operator bool() const override;
private:
    std::unique_ptr<GoogleDocsUploader> gdu;
};

