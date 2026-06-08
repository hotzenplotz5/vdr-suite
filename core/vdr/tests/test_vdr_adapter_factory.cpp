#include "VdrAdapterFactory.h"
#include "IVdrAdapter.h"
#include "VdrConfig.h"
#include "VdrStatus.h"

#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>

static void test_factory_creates_external_adapter()
{
    VdrConfig config;
    config.enabled = true;
    config.mode = "external";

    std::unique_ptr<IVdrAdapter> adapter = VdrAdapterFactory::create(config);

    assert(adapter != nullptr);
}

static void test_factory_created_external_adapter_reports_status()
{
    VdrConfig config;
    config.enabled = true;
    config.mode = "external";
    config.host = "127.0.0.1";
    config.port = 6419;

    std::unique_ptr<IVdrAdapter> adapter = VdrAdapterFactory::create(config);
    VdrStatus status = adapter->getStatus();

    assert(status.enabled == true);
    assert(status.mode == "external");
    assert(status.host == "127.0.0.1");
    assert(status.port == 6419);
    assert(status.state == "configured");
}

static void test_factory_creates_mock_adapter()
{
    VdrConfig config;
    config.enabled = true;
    config.mode = "mock";

    std::unique_ptr<IVdrAdapter> adapter = VdrAdapterFactory::create(config);
    VdrStatus status = adapter->getStatus();

    assert(adapter != nullptr);
    assert(status.enabled == true);
    assert(status.mode == "mock");
    assert(status.host == "mock");
    assert(status.port == 0);
    assert(status.state == "connected");
}

static void test_factory_rejects_unknown_mode()
{
    VdrConfig config;
    config.mode = "unknown";

    bool exceptionThrown = false;

    try {
        std::unique_ptr<IVdrAdapter> adapter = VdrAdapterFactory::create(config);
        (void)adapter;
    } catch (const std::invalid_argument&) {
        exceptionThrown = true;
    }

    assert(exceptionThrown == true);
}

int main()
{
    test_factory_creates_external_adapter();
    test_factory_created_external_adapter_reports_status();
    test_factory_creates_mock_adapter();
    test_factory_rejects_unknown_mode();

    return 0;
}
