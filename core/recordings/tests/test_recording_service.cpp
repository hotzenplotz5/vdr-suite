#include "Database.h"
#include "RecordingRepository.h"
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

    RecordingRepository repository(db);
    RecordingService service(repository);

    auto recordings = service.getAllRecordings();

    if (recordings.empty())
    {
        std::cerr << "no recordings found\n";
        return 1;
    }

    for (const auto& recording : recordings)
    {
        std::cout
            << recording.id << " | "
            << recording.title << " | "
            << recording.recordingFormat
            << std::endl;
    }

    return 0;
}
