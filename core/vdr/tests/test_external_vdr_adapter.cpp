#include "ExternalVdrAdapter.h"

#include <cassert>
#include <string>

int main()
{
    ExternalVdrAdapter adapter;

    const VdrStatus status = adapter.getStatus();

    assert(status.enabled == true);
    assert(status.mode == "external");
    assert(status.host == "127.0.0.1");
    assert(status.port == 6419);
    assert(status.state == "configured");

    return 0;
}
