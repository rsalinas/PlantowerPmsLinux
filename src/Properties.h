#pragma once

#include <map>
#include <string>
#include <vector>

class Properties {
public:
    Properties(const std::string& filename);
    ~Properties();
    std::string getString(const std::string& key,
                          const std::string& defaultValue = NO_DEFAULT_VALUE) const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    void dump() const;
    class KeyNotFound : public std::exception {

    };

private:
    std::map<std::string, std::string> map_;
    static const std::string NO_DEFAULT_VALUE;
};


