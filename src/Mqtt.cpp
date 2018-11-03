#include "Mqtt.h"

#include "Properties.h"
#include <iostream>
#include <cstring> //strlen

using namespace std;

MqttUploader::MqttUploader(std::unique_ptr<mosqpp::mosquittopp> mosq, const std::string& prefix)
    : mosq_(std::move(mosq))
    , prefix_(prefix)
    , statusTopic(prefix + "/status")
{

}

MqttUploader::~MqttUploader() {
//    std::string topic = prefix_ + "/status";
//    std::string payload = "offline";
//    auto err = mosq_->publish(nullptr, topic.c_str(), payload.size(), payload.c_str(), 0, false);

    auto err = mosq_->disconnect();
    mosq_->loop();
    if (err != MOSQ_ERR_SUCCESS) {
        clog << "Error disconnecting mosquitto: " << err << endl;
    }

}
void MqttUploader::update(const unsigned short* data,
                          const std::vector<Item>& metadata) {
    size_t pos = 0;
    for (const auto& item : metadata) {
        clog << item.name << " ->> " << swapShort(data[pos]) << " " << endl;
        int id;
        std::string topic = prefix_ + "/" + item.name;
        std::string payload = std::to_string(swapShort(data[pos]));
        int qos = 0;
        clog << "Sending to topic: " << topic << " data = " << payload << endl;
        int err = mosq_->publish(&id, topic.c_str(), payload.size(), payload.c_str(), qos, false);
        if (err != MOSQ_ERR_SUCCESS) {
            clog << "Mosquitto error: " << err << endl;
        }
        mosq_->loop();

        pos++;
    }
}

bool MqttUploaderBackend::initialize(const Properties& props) {
    clog << "initialize mqtt" << endl;
    auto id = props.getString("mqtt.id", "PMS34223");
    auto host = props.getString("mqtt.host", "");
    auto port = props.getInt("mqtt.port", 1883);
    auto prefix = props.getString("mqtt.prefix", "");
    auto user = props.getString("mqtt.user", "");
    auto passwd = props.getString("mqtt.passwd", "");
    if (host.empty() || prefix.empty()) {
        clog << "mqtt not configured" << endl;
        return false;
    }
    if (mosqpp::lib_init() != MOSQ_ERR_SUCCESS) {
        clog << "Could not initialize mosquitto" << endl;
    }

    std::unique_ptr<mosqpp::mosquittopp> mosq{new mosqpp::mosquittopp};
    clog << "Connecting to " << host << ":" << port << endl;
    clog << "ID: " << id << endl;

    auto err = mosq->connect(host.c_str(), port,  60);
    if (err!= MOSQ_ERR_SUCCESS) {
        clog << "Error connecting " << err << endl;
        return false;
    }
    std::string topic = prefix + "/" + "status";
    auto offlineMessage = "error";
    err = mosq->will_set(topic.c_str(), strlen(offlineMessage), offlineMessage, 2, true);
    if (err != MOSQ_ERR_SUCCESS) {
        clog << "Error setting last will" << endl;
        return false;
    }
    clog << "Connected :)" << endl;
    auto data = "online";
    err = mosq->publish(nullptr, topic.c_str(), strlen(data), data, 0, false);
    while (mosq->want_write()) {
        clog << "looping" << endl;
        mosq->loop();
    }
    clog << "sent: " << err << endl;
    if (user.size() && passwd.size()) {
        err = mosq->username_pw_set(user.c_str(), user.c_str());
        if (err != MOSQ_ERR_SUCCESS) {
            clog << "Bad credential" << endl;
            return false;
        }
    }
    uploader_.reset(new MqttUploader{std::move(mosq), prefix });
    return true;
}

bool MqttUploaderBackend::registerCallback(Pms& pms) {
    pms.addListener(*uploader_);
    return true;
}

MqttUploaderBackend::operator bool() const {
    return uploader_.get() != nullptr;
}

void MqttUploader::onError(const char* message) {
    clog << "mqtt saw error: " << message << endl;
    auto data = message;
    auto err = mosq_->publish(nullptr, statusTopic.c_str(), strlen(data), data, 0, false);
    if (err != MOSQ_ERR_SUCCESS) {
        clog << "error notifying error via mqtt" << endl;
    }
    mosq_->loop();
}
