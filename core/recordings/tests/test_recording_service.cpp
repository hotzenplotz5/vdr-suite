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
        return 1;
    }

    RecordingRepository recordingRepository(db);
    MetadataRepository metadataRepository(db);

    RecordingService service(
        recordingRepository,
        metadataRepository
    );

    auto results =
        service.findRecordingsByTitle("Tat");

    if (results.empty())
    {
        std::cerr << "search failed\n";
        return 1;
    }

    for (const auto& recording : results)
    {
        std::cout
            << recording.id << " | "
            << recording.title
            << std::endl;
    }

    return 0;
}
