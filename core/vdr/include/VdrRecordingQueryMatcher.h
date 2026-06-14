#pragma once

#include "VdrRecording.h"
#include "VdrRecordingQuery.h"

class VdrRecordingQueryMatcher
{
public:
    bool matches(
        const VdrRecording& recording,
        const VdrRecordingQuery& query) const;
};
