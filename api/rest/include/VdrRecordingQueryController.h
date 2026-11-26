#pragma once

#include "DashboardController.h"
#include "VdrRecordingQueryResult.h"
#include "ManualRecordingMetadataAssignmentRepository.h"

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

class VdrRecordingQueryResultJsonSerializer;
class VdrRecordingQueryService;

class VdrRecordingQueryController
{
public:
    using ManualMetadataBatchLookup =
        std::function<
            std::map<std::string, ManualRecordingMetadataAssignment>(
                const std::string& backendId)>;

    VdrRecordingQueryController(
        VdrRecordingQueryService& queryService,
        VdrRecordingQueryResultJsonSerializer& jsonSerializer);

    VdrRecordingQueryController(
        VdrRecordingQueryService& queryService,
        VdrRecordingQueryResultJsonSerializer& jsonSerializer,
        ManualMetadataBatchLookup manualMetadataBatchLookup);

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
        int offset,
        int movieReleaseYearFrom = 0,
        int movieReleaseYearTo = 0);

    struct HomeItem {
        VdrRecording recording;
        std::string title, subtitle;
    };
    struct HomePage {
        std::vector<HomeItem> items;
        int totalCount = 0;
    };
    HomePage homePage(const std::string& backend, bool movies, int yearFrom,
        int yearTo, int limit, int offset);

private:
    using Assignments = std::map<std::string, ManualRecordingMetadataAssignment>;
    std::pair<VdrRecordingQueryResult, Assignments> queryWithAssignments(
        const std::string& title, const std::string& backend, const std::string& path,
        const std::string& sort, const std::string& order, const std::string& from,
        const std::string& to, int durationMin, int durationMax, int limit,
        int offset, int movieReleaseYearFrom, int movieReleaseYearTo);
    VdrRecordingQueryService& queryService_;
    VdrRecordingQueryResultJsonSerializer& jsonSerializer_;
    ManualMetadataBatchLookup manualMetadataBatchLookup_;
};
