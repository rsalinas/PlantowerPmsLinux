#pragma once

#include <cstring>
#include <functional>
#include <vector>

struct ConfigParserBase  {
public:
    bool parse(const char ** argv);

protected:
    struct Option {
        std::vector<const char*> modifiers_;
        const char* argDesc_;
        const char* desc_;
        std::function<bool(const char**&arg)> f_;
    };
    ConfigParserBase(std::vector<Option> options)
        : optionMap_(std::move(options)) {

    }
    void showHelp();

private:
    const std::vector<Option> optionMap_;

};
