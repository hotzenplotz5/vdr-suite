#pragma once

#include "VdrRecordingQueryResult.h"

#include <string>

class VdrRecordingQueryResultJsonSerializer
{
public:
    std::string serialize(
        const VdrRecordingQueryResult& result) const;
};
