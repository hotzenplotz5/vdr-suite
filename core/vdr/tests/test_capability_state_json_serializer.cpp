#include "CapabilityStateJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    CapabilityStateJsonSerializer serializer;

    std::string availableJson =
        serializer.serialize(
            CapabilityState::available("recordings.read"));

    assert(availableJson.find("\"capability\":\"recordings.read\"") != std::string::npos);
    assert(availableJson.find("\"supported\":true") != std::string::npos);
    assert(availableJson.find("\"availability\":\"available\"") != std::string::npos);
    assert(availableJson.find("\"availableNow\":true") != std::string::npos);
    assert(availableJson.find("\"reason\":\"\"") != std::string::npos);

    std::string unsupportedJson =
        serializer.serialize(
            CapabilityState::unsupported(
                "live.stream",
                "backend does not support live streaming"));

    assert(unsupportedJson.find("\"capability\":\"live.stream\"") != std::string::npos);
    assert(unsupportedJson.find("\"supported\":false") != std::string::npos);
    assert(unsupportedJson.find("\"availability\":\"unsupported\"") != std::string::npos);
    assert(unsupportedJson.find("\"availableNow\":false") != std::string::npos);
    assert(unsupportedJson.find("\"reason\":\"backend does not support live streaming\"") != std::string::npos);

    std::string notConfiguredJson =
        serializer.serialize(
            CapabilityState::notConfigured(
                "recordings.read",
                "no backend configured"));

    assert(notConfiguredJson.find("\"availability\":\"notConfigured\"") != std::string::npos);

    std::string offlineJson =
        serializer.serialize(
            CapabilityState::offline(
                "timers.read",
                "backend offline"));

    assert(offlineJson.find("\"availability\":\"offline\"") != std::string::npos);

    std::string temporarilyUnavailableJson =
        serializer.serialize(
            CapabilityState::temporarilyUnavailable(
                "events.read",
                "backend refresh in progress"));

    assert(temporarilyUnavailableJson.find("\"availability\":\"temporarilyUnavailable\"") != std::string::npos);

    std::cout
        << "test_capability_state_json_serializer passed"
        << std::endl;

    return 0;
}
