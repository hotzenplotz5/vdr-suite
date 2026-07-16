#pragma once

#include "VdrRecording.h"

#include <string>
#include <vector>

class RestfulApiRecordingMetadataEnricher
{
public:
    static void enrich(
        const std::string& recordingsJson,
        std::vector<VdrRecording>& recordings);
};
