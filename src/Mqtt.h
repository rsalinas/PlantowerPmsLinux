#pragma once

#include <memory>
#include <mosquittopp.h>

#include "Pms.h"

class MqttUploader : public PmsMeasurementCallback  {
public:
    MqttUploader(std::unique_ptr<mosqpp::mosquittopp> mosq, const std::string& prefix);
    void update(const unsigned short* data, const std::vector<Item>& metadata) override;
    void onError(const char* message) override;
    ~MqttUploader();

private:
    std::unique_ptr<mosqpp::mosquittopp> mosq_;
    const std::string prefix_;
    const std::string statusTopic;
};

class MqttUploaderBackend : public Backend {
public:
    bool initialize(const Properties& props) override;
    bool registerCallback(Pms& pms) override;
    operator bool() const override;
private:
    std::unique_ptr<MqttUploader> uploader_;
};
