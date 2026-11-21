#include "Database.h"
#include "TestDatabasePath.h"
#include "RecordingRepository.h"

#include <iostream>

int main()
{
    Database db;

    if (!db.open(vdrSuiteTestDatabasePath()))
    {
        std::cerr << "database open failed\n";
        return 1;
    }

    RecordingRepository repository(db);

    auto recordings =
        repository.getAllRecordings();

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
            << recording.recordingFormat << " | "
            << recording.recordingPath
            << std::endl;
    }

    return 0;
}
