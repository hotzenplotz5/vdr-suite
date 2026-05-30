#pragma once

#include "Recording.h"
#include "RecordingDetails.h"

#include <vector>

class RecordingRepository;
class MetadataRepository;

class RecordingService
{
public:
    RecordingService(
        RecordingRepository& recordingRepository,
        MetadataRepository& metadataRepository
    );

    std::vector<Recording> getAllRecordings();
    RecordingDetails getRecordingDetails(int id);

private:
    RecordingRepository& recordingRepository_;
    MetadataRepository& metadataRepository_;
};
