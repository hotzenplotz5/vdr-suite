#include "ILiveTransport.h"
#include "LiveTransportFactory.h"
#include "LiveUpdateEvent.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <stdexcept>

static void test_factory_creates_test_live_transport()
{
    std::unique_ptr<ILiveTransport> transport =
        LiveTransportFactory::create("test");

    assert(transport != nullptr);

    transport->publish(LiveUpdateEvent(
        1,
        10,
        {"status"},
        "home-vdr"));
}

static void test_factory_rejects_unknown_live_transport_mode()
{
    bool failed = false;

    try {
        LiveTransportFactory::create("sse");
    } catch (const std::invalid_argument&) {
        failed = true;
    }

    assert(failed == true);
}

int main()
{
    test_factory_creates_test_live_transport();
    test_factory_rejects_unknown_live_transport_mode();

    std::cout
        << "test_live_transport_factory passed"
        << std::endl;

    return 0;
}
