#pragma once

#include "DashboardController.h"

#include <string>

class VdrRecordingQueryResultJsonSerializer;
class VdrRecordingQueryService;

class VdrRecordingQueryController
{
public:
    VdrRecordingQueryController(
        VdrRecordingQueryService& queryService,
        VdrRecordingQueryResultJsonSerializer& jsonSerializer);

    ApiResponse getRecordings();

    ApiResponse getRecordings(
        const std::string& title,
        int limit,
        int offset);

private:
    VdrRecordingQueryService& queryService_;
    VdrRecordingQueryResultJsonSerializer& jsonSerializer_;
};
