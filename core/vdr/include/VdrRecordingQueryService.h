#pragma once

#include "VdrRecording.h"
#include "VdrRecordingQuery.h"
#include "VdrRecordingQueryResult.h"

#include <string>
#include <vector>

class VdrRecordingCacheRepository;
class VdrService;

class VdrRecordingQueryService
{
public:
    explicit VdrRecordingQueryService(
        VdrService& vdrService,
        VdrRecordingCacheRepository* recordingCacheRepository = nullptr,
        const std::string& defaultBackendId = "default");

    VdrRecordingQueryResult queryRecordings(
        const VdrRecordingQuery& query) const;

    bool findRecordingById(
        const std::string& backendId,
        const std::string& recordingId,
        VdrRecording& recording) const;

private:
    VdrService& vdrService_;
    VdrRecordingCacheRepository* recordingCacheRepository_;
    std::string defaultBackendId_;

    std::vector<VdrRecording> loadRecordings(
        const VdrRecordingQuery& query) const;

    std::string effectiveBackendId(
        const VdrRecordingQuery& query) const;
};
