#pragma once

#include "VdrRecording.h"

#include <string>
#include <vector>

class RestfulApiRecordingMapper {
public:
    static std::vector<VdrRecording> parseRecordings(const std::string& json);
};
