#include "CliParser.h"

#include <iostream>
#include <algorithm>

using namespace std;

bool ConfigParserBase::parse(const char ** argv) {
    for (const char**ptr = argv+1; *ptr; ptr++) {
        auto pos = std::find_if(optionMap_.begin(),
                                optionMap_.end(),
                                [ptr](const Option& option) {
            return std::find_if(
                        option.modifiers_.begin(), option.modifiers_.end(),
                        std::bind(strcmp, *ptr, std::placeholders::_1))
                    != option.modifiers_.end();
        });
        if (pos != optionMap_.end()) {
            if (!pos->f_(ptr))
                return false;
        } else {
            clog << "Bad option: " << *ptr << endl;
            return false;
        }
    }
    return true;
}

void ConfigParserBase::showHelp() {
    clog << "Options:" << endl;
    std::vector<std::string> modifierLists;
    size_t maxModifierListLength = 0;
    for (auto& option : optionMap_) {
        std::string modifierList;
        for (auto modifier : option.modifiers_) {
            if (!modifierList.empty())
                modifierList += "|";
            modifierList += modifier;
        }
        if (option.argDesc_)
            modifierList += std::string{" <"} + option.argDesc_ + ">";
        if (modifierList.size() > maxModifierListLength)
            maxModifierListLength = modifierList.size();
        modifierLists.emplace_back(modifierList);
    }
    for (auto i = 0; i < optionMap_.size(); ++i) {
        clog << std::string(2, ' ')
             << modifierLists[i]
                << std::string(maxModifierListLength - modifierLists[i].size(), ' ')
                << " - "
                << optionMap_[i].desc_
                << endl;
    }
    exit(0);
}
