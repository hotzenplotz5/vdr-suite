#include "Database.h"
#include "MetadataRepository.h"
#include "MetadataService.h"

#include <iostream>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-test.db"))
    {
        std::cerr << "database open failed\n";
        return 1;
    }

    MetadataRepository repository(db);
    MetadataService service(repository);

    auto metadataItems = service.getAllMetadata();

    if (metadataItems.empty())
    {
        std::cerr << "no metadata found\n";
        return 1;
    }

    for (const auto& metadata : metadataItems)
    {
        std::cout
            << metadata.id << " | "
            << metadata.title << " | "
            << metadata.genre << " | "
            << metadata.year
            << std::endl;
    }

    return 0;
}
