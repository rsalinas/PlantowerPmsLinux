#include "GoogleDocs.h"

#include <iostream>
#include <sstream>
#include "Properties.h"

using namespace std;

GoogleDocsUploader::GoogleDocsUploader(const char* url, const std::vector<google_entry_t>& itemList)
    : url_(url), itemList_(itemList) {
}

void GoogleDocsUploader::update(const unsigned short* data, const std::vector<Item>& metadata) {
    auto now = time(NULL);

    // Only once per interval
    if (now / intervalSeconds_ == lastSuccessfulUpdate_ / intervalSeconds_) {
        return;
    }

    std::string body;
    for (auto i = 0u ; i < metadata.size(); i++) {
        body += "entry."
                + std::to_string(itemList_[i])
                + "="
                + std::to_string(swapShort(data[i])) + "&";
        //        clog << "XXX" << metadata[i].name << "=" << swapShort(data[i]) << ' ';
    }

    char buf[4096];
    snprintf(buf, sizeof buf, "curl '%s' -d '%s' --silent --output /dev/null ", url_.c_str(), body.c_str());
    clog << "HTTP request: " << buf << endl;
    if (system(buf) != 0) {
        clog << "Error uploading to Google Forms" << endl;
        return;
    }
    lastSuccessfulUpdate_ = now;
}


std::vector<GoogleDocsUploader::google_entry_t> GoogleDocsUploader::splitOnToken(const std::string& str, char delimiter) {
    std::stringstream ss{str};
    std::string segment;
    std::vector<google_entry_t> ret;
    while(std::getline(ss, segment, delimiter)) {
        google_entry_t item;
        std::stringstream{segment} >> item;
        ret.push_back(item);
    }
    return ret;
}


bool GoogleDocsUploaderBackend::initialize(const Properties& props)  {
    auto googleDocs_url = props.getString("googledocs.url", "");
    auto googleDocs_items = props.getString("googledocs.items", "");
    if (!googleDocs_url.empty() && !googleDocs_items.empty()) {
        auto items = GoogleDocsUploader::splitOnToken(googleDocs_items, ',');
        if (false)
            for (auto i : items)
                clog << "item: " << i << endl;
        gdu.reset(new GoogleDocsUploader{googleDocs_url.c_str(), items});
        if (items.empty()) {
            clog << "Missing items for Google" << endl;
        }
        return true;
    }
    clog << "GoogleDocsUploaderBackend not configured" << endl;
    return false;
}
bool GoogleDocsUploaderBackend::registerCallback(Pms& pms)  {
    pms.addListener(*gdu);
    return true;
}

GoogleDocsUploaderBackend::operator bool() const {
    return gdu.get() != nullptr;
}
