#include "ExternalVdrAdapter.h"
#include "IVdrAdapter.h"

#include <cassert>
#include <memory>
#include <string>

static void test_external_vdr_adapter_reports_disabled_state()
{
    VdrConfig config;
    config.enabled = false;

    ExternalVdrAdapter adapter(config);
    VdrStatus status = adapter.getStatus();

    assert(status.enabled == false);
    assert(status.state == "disabled");
}

static void test_external_vdr_adapter_reports_configured_state()
{
    VdrConfig config;
    config.enabled = true;

    ExternalVdrAdapter adapter(config);
    VdrStatus status = adapter.getStatus();

    assert(status.enabled == true);
    assert(status.state == "configured");
}

static void test_external_vdr_adapter_preserves_config_values()
{
    VdrConfig config;
    config.enabled = true;
    config.mode = "external";
    config.host = "127.0.0.1";
    config.port = 6419;

    ExternalVdrAdapter adapter(config);
    VdrStatus status = adapter.getStatus();

    assert(status.enabled == true);
    assert(status.mode == "external");
    assert(status.host == "127.0.0.1");
    assert(status.port == 6419);
}

static void test_external_vdr_adapter_can_be_used_through_interface()
{
    VdrConfig config;
    config.enabled = true;

    std::unique_ptr<IVdrAdapter> adapter =
        std::make_unique<ExternalVdrAdapter>(config);

    VdrStatus status = adapter->getStatus();

    assert(status.enabled == true);
    assert(status.state == "configured");
}

int main()
{
    test_external_vdr_adapter_reports_disabled_state();
    test_external_vdr_adapter_reports_configured_state();
    test_external_vdr_adapter_preserves_config_values();
    test_external_vdr_adapter_can_be_used_through_interface();

    return 0;
}
