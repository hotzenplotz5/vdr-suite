#pragma once

#include "RecordingActionRequestPreviewResult.h"

#include <string>

class RecordingActionRequestPreviewResultJsonSerializer
{
public:
    std::string serialize(
        const RecordingActionRequestPreviewResult& result) const;
};
