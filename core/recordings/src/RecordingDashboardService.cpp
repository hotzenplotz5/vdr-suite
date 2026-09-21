#include "RecordingDashboardService.h"

#include "MetadataRepository.h"
#include "RecordingRepository.h"

RecordingDashboardService::RecordingDashboardService(
    RecordingRepository& recordingRepository,
    MetadataRepository& metadataRepository)
    : recordingRepository_(recordingRepository),
      metadataRepository_(metadataRepository)
{
}

RecordingDashboardSummary
RecordingDashboardService::getSummary()
{
    RecordingDashboardSummary summary;

    const auto recordings =
        recordingRepository_.getAllRecordings();

    const auto metadata =
        metadataRepository_.getAllMetadata();

    summary.recordingsTotal =
        static_cast<int>(recordings.size());

    summary.recordingsWithMetadata =
        static_cast<int>(metadata.size());

    summary.recordingsWithoutMetadata =
        summary.recordingsTotal -
        summary.recordingsWithMetadata;

    for (const auto& recording : recordings)
    {
        if (recording.id > summary.latestRecordingId)
        {
            summary.latestRecordingId =
                recording.id;

            summary.latestRecordingTitle =
                recording.title;
        }
    }

    return summary;
}
