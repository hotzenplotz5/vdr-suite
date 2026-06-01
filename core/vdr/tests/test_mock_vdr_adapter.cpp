#include "MockVdrAdapter.h"
#include "IVdrAdapter.h"

#include <cassert>
#include <memory>
#include <string>

static void test_mock_vdr_adapter_reports_connected_state()
{
    MockVdrAdapter adapter;
    VdrStatus status = adapter.getStatus();

    assert(status.enabled == true);
    assert(status.mode == "mock");
    assert(status.host == "mock");
    assert(status.port == 0);
    assert(status.state == "connected");
}

static void test_mock_vdr_adapter_can_be_used_through_interface()
{
    std::unique_ptr<IVdrAdapter> adapter =
        std::make_unique<MockVdrAdapter>();

    VdrStatus status = adapter->getStatus();

    assert(status.enabled == true);
    assert(status.mode == "mock");
    assert(status.state == "connected");
}

int main()
{
    test_mock_vdr_adapter_reports_connected_state();
    test_mock_vdr_adapter_can_be_used_through_interface();

    return 0;
}
