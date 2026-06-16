#pragma once

#include "RecordingActionSafetyResult.h"

#include <string>

class RecordingActionSafetyResultJsonSerializer
{
public:
    std::string serialize(
        const RecordingActionSafetyResult& result) const;
};
