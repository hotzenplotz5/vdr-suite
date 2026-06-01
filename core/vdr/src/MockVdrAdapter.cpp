#include "MockVdrAdapter.h"

MockVdrAdapter::MockVdrAdapter()
{
}

VdrStatus MockVdrAdapter::getStatus() const
{
    VdrStatus status;
    status.enabled = true;
    status.mode = "mock";
    status.host = "mock";
    status.port = 0;
    status.state = "connected";

    return status;
}
