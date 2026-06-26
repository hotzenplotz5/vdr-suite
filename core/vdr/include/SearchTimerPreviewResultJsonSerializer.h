#pragma once

#include "SearchTimerPreviewResult.h"

#include <sstream>
#include <string>
#include <vector>

class SearchTimerPreviewResultJsonSerializer {
public:
    std::string serialize(
        const SearchTimerPreviewResult& result) const;

private:
    static void appendQuoted(
        std::ostringstream& json,
        const std::string& value);

    static void appendStringArray(
        std::ostringstream& json,
        const std::vector<std::string>& values);

    static const char* stateToJson(
        SearchTimerState state);
};
