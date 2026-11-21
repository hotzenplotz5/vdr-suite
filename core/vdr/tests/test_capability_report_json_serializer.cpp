#include "CapabilityReportJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::vector<CapabilityState> states;
    states.push_back(CapabilityState::available("recordings.read"));
    states.push_back(
        CapabilityState::unsupported(
            "timers.create",
            "capability unsupported by backend"));

    CapabilityReport report(
        "living-room",
        states);

    CapabilityReportJsonSerializer serializer;
    std::string json = serializer.serialize(report);

    assert(json.find("\"backendId\":\"living-room\"") != std::string::npos);
    assert(json.find("\"capabilities\":[") != std::string::npos);
    assert(json.find("\"capability\":\"recordings.read\"") != std::string::npos);
    assert(json.find("\"supported\":true") != std::string::npos);
    assert(json.find("\"availability\":\"available\"") != std::string::npos);
    assert(json.find("\"availableNow\":true") != std::string::npos);
    assert(json.find("\"capability\":\"timers.create\"") != std::string::npos);
    assert(json.find("\"supported\":false") != std::string::npos);
    assert(json.find("\"availability\":\"unsupported\"") != std::string::npos);
    assert(json.find("\"availableNow\":false") != std::string::npos);
    assert(json.find("\"reason\":\"capability unsupported by backend\"") != std::string::npos);

    CapabilityReport emptyReport(
        "empty-backend",
        {});

    std::string emptyJson = serializer.serialize(emptyReport);

    assert(emptyJson == "{\"backendId\":\"empty-backend\",\"capabilities\":[]}");

    std::cout
        << "test_capability_report_json_serializer passed"
        << std::endl;

    return 0;
}
