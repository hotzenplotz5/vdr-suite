#include "BackendNode.h"

#include <cassert>
#include <iostream>

int main()
{
    BackendNode node;

    assert(node.backendId == "default");
    assert(node.backendName == "Default VDR");
    assert(node.backendType == "vdr");
    assert(node.enabled);
    assert(!node.online);

    assert(node.connection.enabled);
    assert(node.connection.mode == "external");
    assert(node.connection.host == "127.0.0.1");
    assert(node.connection.port == 6419);

    assert(!node.capabilities.snapshotRead);
    assert(!node.capabilities.statusRead);
    assert(!node.capabilities.healthRead);
    assert(!node.capabilities.recordingsRead);
    assert(!node.capabilities.timersRead);
    assert(!node.capabilities.channelsRead);
    assert(!node.capabilities.eventsRead);

    std::cout
        << "test_backend_node passed"
        << std::endl;

    return 0;
}
