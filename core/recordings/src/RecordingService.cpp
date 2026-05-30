#include "RecordingService.h"
#include "RecordingRepository.h"
#include "MetadataRepository.h"

RecordingService::RecordingService(
    RecordingRepository& recordingRepository,
    MetadataRepository& metadataRepository
)
    : recordingRepository_(recordingRepository),
      metadataRepository_(metadataRepository)
{
}

std::vector<Recording> RecordingService::getAllRecordings()
{
    return recordingRepository_.getAllRecordings();
}

RecordingDetails RecordingService::getRecordingDetails(int id)
{
    RecordingDetails details;

    auto recording =
        recordingRepository_.getRecordingById(id);

    if (!recording)
    {
        return details;
    }

    details.recording = *recording;

    auto metadata =
        metadataRepository_.getMetadataForRecording(id);

    if (metadata)
    {
        details.metadata = *metadata;
        details.hasMetadata = true;
    }

    return details;
}
