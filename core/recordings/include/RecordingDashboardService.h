#pragma once

#include "RecordingDashboardSummary.h"

class RecordingRepository;
class MetadataRepository;

class RecordingDashboardService
{
public:
    RecordingDashboardService(
        RecordingRepository& recordingRepository,
        MetadataRepository& metadataRepository);

    RecordingDashboardSummary getSummary();

private:
    RecordingRepository& recordingRepository_;
    MetadataRepository& metadataRepository_;
};
