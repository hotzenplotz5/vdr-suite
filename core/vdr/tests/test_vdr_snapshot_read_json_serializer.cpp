#include "VdrSnapshotReadJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

static void test_snapshot_read_serializer_serializes_status()
{
    VdrStatus status;
    status.enabled = true;
    status.mode = "snapshot-read";
    status.host = "vdr-host";
    status.port = 8002;
    status.state = "connected";

    VdrSnapshotReadJsonSerializer serializer;
    std::string json = serializer.serializeStatus(status);

    assert(json.find("\"enabled\":true") != std::string::npos);
    assert(json.find("\"mode\":\"snapshot-read\"") != std::string::npos);
    assert(json.find("\"host\":\"vdr-host\"") != std::string::npos);
    assert(json.find("\"port\":8002") != std::string::npos);
    assert(json.find("\"state\":\"connected\"") != std::string::npos);

    std::cout << json << std::endl;
}

static void test_snapshot_read_serializer_serializes_empty_domain_lists()
{
    VdrSnapshotReadJsonSerializer serializer;

    assert(serializer.serializeRecordings({}) == "{\"recordings\":[]}");
    assert(serializer.serializeTimers({}) == "{\"timers\":[]}");
    assert(serializer.serializeChannels({}) == "{\"channels\":[]}");
    assert(serializer.serializeEvents({}) == "{\"events\":[]}");
}

int main()
{
    test_snapshot_read_serializer_serializes_status();
    test_snapshot_read_serializer_serializes_empty_domain_lists();

    std::cout
        << "test_vdr_snapshot_read_json_serializer passed"
        << std::endl;

    return 0;
}
