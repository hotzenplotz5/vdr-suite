#include "VdrRecordingQueryController.h"

#include "MockVdrAdapter.h"
#include "VdrRecordingQueryResultJsonSerializer.h"
#include "VdrRecordingQueryService.h"
#include "VdrService.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrRecordingQueryService queryService(vdrService);
    VdrRecordingQueryResultJsonSerializer jsonSerializer;

    VdrRecordingQueryController controller(
        queryService,
        jsonSerializer);

    ApiResponse allResponse =
        controller.getRecordings();

    assert(allResponse.statusCode == 200);
    assert(allResponse.contentType == "application/json");
    assert(allResponse.body.find("\"totalCount\":2") != std::string::npos);
    assert(allResponse.body.find("\"returnedCount\":2") != std::string::npos);
    assert(allResponse.body.find("\"title\":\"Tagesschau\"") != std::string::npos);
    assert(allResponse.body.find("\"title\":\"Tatort\"") != std::string::npos);

    ApiResponse limitedResponse =
        controller.getRecordings(
            "",
            "",
            1,
            1);

    assert(limitedResponse.statusCode == 200);
    assert(limitedResponse.body.find("\"totalCount\":2") != std::string::npos);
    assert(limitedResponse.body.find("\"returnedCount\":1") != std::string::npos);
    assert(limitedResponse.body.find("\"limit\":1") != std::string::npos);
    assert(limitedResponse.body.find("\"offset\":1") != std::string::npos);
    assert(limitedResponse.body.find("\"title\":\"Tatort\"") != std::string::npos);

    ApiResponse titleResponse =
        controller.getRecordings(
            "tator",
            "",
            10,
            0);

    assert(titleResponse.statusCode == 200);
    assert(titleResponse.body.find("\"totalCount\":1") != std::string::npos);
    assert(titleResponse.body.find("\"returnedCount\":1") != std::string::npos);
    assert(titleResponse.body.find("\"title\":\"Tatort\"") != std::string::npos);
    assert(titleResponse.body.find("\"title\":\"Tagesschau\"") == std::string::npos);

    ApiResponse pathResponse =
        controller.getRecordings(
            "",
            "Tagesschau",
            10,
            0);

    assert(pathResponse.statusCode == 200);
    assert(pathResponse.body.find("\"totalCount\":1") != std::string::npos);
    assert(pathResponse.body.find("\"returnedCount\":1") != std::string::npos);
    assert(pathResponse.body.find("\"title\":\"Tagesschau\"") != std::string::npos);
    assert(pathResponse.body.find("\"title\":\"Tatort\"") == std::string::npos);

    std::cout
        << "test_vdr_recording_query_controller passed"
        << std::endl;

    return 0;
}
