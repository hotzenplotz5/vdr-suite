#pragma once

#include "PersonQuery.h"
#include "RecordingPersonSearchResult.h"

#include <string>

class VdrRecordingCacheRepository;
class VdrRecordingNativeMetadataRepository;

class VdrRecordingNativePersonSearchService
{
public:
    VdrRecordingNativePersonSearchService(
        VdrRecordingNativeMetadataRepository& metadataRepository,
        VdrRecordingCacheRepository& recordingCacheRepository);

    RecordingPersonSearchResult search(
        const std::string& backendId,
        const PersonQuery& query,
        int limit,
        int offset) const;

private:
    VdrRecordingNativeMetadataRepository& metadataRepository_;
    VdrRecordingCacheRepository& recordingCacheRepository_;
};
