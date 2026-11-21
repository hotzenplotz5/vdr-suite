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

    std::vector<Recording> findRecordingsByTitle(
        const std::string& title);

private:
    RecordingRepository& recordingRepository_;
    MetadataRepository& metadataRepository_;
};
