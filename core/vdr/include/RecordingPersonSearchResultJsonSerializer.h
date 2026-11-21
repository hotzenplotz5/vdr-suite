#pragma once

#include "RecordingPersonSearchResult.h"

#include <string>

class RecordingPersonSearchResultJsonSerializer
{
public:
    std::string serialize(
        const RecordingPersonSearchResult& result) const;
};
