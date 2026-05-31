#include "Database.h"
#include "MetadataRepository.h"
#include "RecordingDashboardService.h"
#include "RecordingRepository.h"

#include <cassert>
#include <iostream>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-test.db"))
    {
        return 1;
    }

    RecordingRepository recordingRepository(db);
    MetadataRepository metadataRepository(db);

    RecordingDashboardService dashboard(
        recordingRepository,
        metadataRepository);

    auto summary =
        dashboard.getSummary();

    assert(summary.recordingsTotal == 2);
    assert(summary.recordingsWithMetadata == 1);
    assert(summary.recordingsWithoutMetadata == 1);

    assert(summary.latestRecordingId == 2);
    assert(summary.latestRecordingTitle == "Tagesschau");

    db.close();

    std::cout
        << "test_recording_dashboard_service passed"
        << std::endl;

    return 0;
}
