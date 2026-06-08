#include "RecordingsController.h"

#include "Database.h"
#include "RecordingRepository.h"

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

    RecordingRepository recordingRepository(db);

    RecordingsController controller(recordingRepository);

    ApiResponse response =
        controller.getRecordings();

    assert(response.statusCode == 200);
    assert(response.contentType == "application/json");

    assert(response.body.find("\"recordings\"") != std::string::npos);
    assert(response.body.find("\"title\":\"Tatort\"") != std::string::npos);
    assert(response.body.find("\"title\":\"Tagesschau\"") != std::string::npos);
    assert(response.body.find("\"recordingFormat\":\"TS\"") != std::string::npos);

    db.close();

    std::cout
        << response.body
        << std::endl;

    std::cout
        << "test_recordings_controller passed"
        << std::endl;

    return 0;
}
