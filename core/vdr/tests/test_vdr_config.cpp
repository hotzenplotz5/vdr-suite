#include "VdrConfig.h"

#include <cassert>

int main()
{
    VdrConfig config;

    assert(config.enabled == true);
    assert(config.mode == "external");
    assert(config.host == "127.0.0.1");
    assert(config.port == 6419);

    return 0;
}
