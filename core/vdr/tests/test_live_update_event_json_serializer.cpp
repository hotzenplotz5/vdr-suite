#include "LiveUpdateEvent.h"
#include "LiveUpdateEventJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

static void test_serializer_writes_live_update_event_json()
{
    LiveUpdateEvent event(
        7,
        42,
        {"channels", "recordings"},
        "home-vdr");

    LiveUpdateEventJsonSerializer serializer;

    const std::string json =
        serializer.serializeEvent(event);

    assert(json.find("\"sequenceNumber\":7") != std::string::npos);
    assert(json.find("\"snapshotGeneration\":42") != std::string::npos);
    assert(json.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(json.find("\"changedDomains\":[\"channels\",\"recordings\"]") != std::string::npos);
}

static void test_serializer_writes_searchtimer_changed_domain()
{
    LiveUpdateEvent event(
        9,
        44,
        {"searchtimers"},
        "home-vdr");

    LiveUpdateEventJsonSerializer serializer;

    const std::string json =
        serializer.serializeEvent(event);

    assert(json.find("\"changedDomains\":[\"searchtimers\"]") != std::string::npos);
}

static void test_serializer_writes_empty_changed_domains()
{
    LiveUpdateEvent event(
        8,
        43,
        {},
        "ferienhaus-vdr");

    LiveUpdateEventJsonSerializer serializer;

    const std::string json =
        serializer.serializeEvent(event);

    assert(json == "{\"sequenceNumber\":8,\"snapshotGeneration\":43,\"backendId\":\"ferienhaus-vdr\",\"changedDomains\":[]}");
}

int main()
{
    test_serializer_writes_live_update_event_json();
    test_serializer_writes_searchtimer_changed_domain();
    test_serializer_writes_empty_changed_domains();

    std::cout
        << "test_live_update_event_json_serializer passed"
        << std::endl;

    return 0;
}
