#include "Database.h"
#include "RecordingRepository.h"
#include "MetadataRepository.h"
#include "RecordingService.h"

#include <iostream>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-test.db"))
    {
        std::cerr << "database open failed\n";
        return 1;
    }

    RecordingRepository recordingRepository(db);
    MetadataRepository metadataRepository(db);

    RecordingService service(
        recordingRepository,
        metadataRepository
    );

    auto details = service.getRecordingDetails(1);

    if (details.recording.id == 0)
    {
        std::cerr << "recording details not found\n";
        return 1;
    }

    std::cout
        << details.recording.title << " | "
        << details.recording.recordingFormat;

    if (details.hasMetadata)
    {
        std::cout
            << " | "
            << details.metadata.genre << " | "
            << details.metadata.year;
    }

    std::cout << std::endl;

    return 0;
}
