#include "Database.h"
#include "RecordingRepository.h"

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

    auto recordings =
        repository.getRecordingTitles();

    for (const auto& title : recordings)
    {
        std::cout << title << std::endl;
    }

    return 0;
}
