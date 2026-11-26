#pragma once

#include "RecordingPersonSearchResult.h"

#include <functional>
#include <string>

class RecordingPersonSearchResultJsonSerializer
{
public:
    using PublicRecordingIdLookup = std::function<std::string(const VdrRecording&)>;

    std::string serialize(
        const RecordingPersonSearchResult& result,
        const PublicRecordingIdLookup& publicRecordingIdLookup = {}) const;
};
