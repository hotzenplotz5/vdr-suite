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
        const std::string& path,
        const std::string& sort,
        const std::string& order,
        const std::string& from,
        const std::string& to,
        int durationMin,
        int durationMax,
        int limit,
        int offset);

    ApiResponse getRecordings(
        const std::string& title,
        const std::string& backend,
        const std::string& path,
        const std::string& sort,
        const std::string& order,
        const std::string& from,
        const std::string& to,
        int durationMin,
        int durationMax,
        int limit,
        int offset);

private:
    VdrRecordingQueryService& queryService_;
    VdrRecordingQueryResultJsonSerializer& jsonSerializer_;
};
