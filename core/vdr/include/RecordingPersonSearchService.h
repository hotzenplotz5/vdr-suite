#pragma once

#include "PersonQuery.h"
#include "RecordingPersonSearchResult.h"
#include "VdrRecording.h"

#include <vector>

class RecordingPersonSearchService
{
public:
    RecordingPersonSearchResult search(
        const std::vector<VdrRecording>& recordings,
        const PersonQuery& query,
        int limit,
        int offset) const;
};
