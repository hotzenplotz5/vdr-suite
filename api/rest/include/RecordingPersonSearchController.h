#pragma once

#include "DashboardController.h"
#include "VdrRecording.h"

#include <string>
#include <vector>

class RecordingPersonSearchResultJsonSerializer;
class RecordingPersonSearchService;

class RecordingPersonSearchController
{
public:
    RecordingPersonSearchController(
        RecordingPersonSearchService& searchService,
        RecordingPersonSearchResultJsonSerializer& jsonSerializer);

    ApiResponse searchRecordingPersons(
        const std::vector<VdrRecording>& recordings,
        const std::string& name,
        const std::string& normalizedName,
        const std::string& role,
        const std::string& source,
        const std::string& providerReference,
        int limit,
        int offset);

private:
    RecordingPersonSearchService& searchService_;
    RecordingPersonSearchResultJsonSerializer& jsonSerializer_;
};
