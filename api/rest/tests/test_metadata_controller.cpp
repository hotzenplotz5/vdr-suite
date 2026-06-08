#include "Database.h"
#include "MetadataRepository.h"
#include "MetadataController.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    Database db;

    if (!db.open("/tmp/vdr-suite-test.db"))
    {
        std::cerr << "database open failed" << std::endl;
        return 1;
    }

    MetadataRepository repository(db);

    MetadataController controller(repository);

    ApiResponse response =
        controller.getMetadata();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");

    assert(response.body.find("\"metadata\"") != std::string::npos);
    assert(response.body.find("\"title\"") != std::string::npos);
    assert(response.body.find("\"genre\"") != std::string::npos);

    db.close();

    std::cout
        << "test_metadata_controller passed"
        << std::endl;

    return 0;
}
