#include "ExternalVdrAdapter.h"
#include "VdrConfig.h"

#include <cassert>
#include <string>

int main()
{
    {
        ExternalVdrAdapter adapter;

        const VdrStatus status = adapter.getStatus();

        assert(status.enabled == true);
        assert(status.mode == "external");
        assert(status.host == "127.0.0.1");
        assert(status.port == 6419);
        assert(status.state == "configured");
    }

    {
        VdrConfig config;
        config.host = "192.168.1.100";
        config.port = 6420;

        ExternalVdrAdapter adapter(config);

        const VdrStatus status = adapter.getStatus();

        assert(status.host == "192.168.1.100");
        assert(status.port == 6420);
    }

    return 0;
}
