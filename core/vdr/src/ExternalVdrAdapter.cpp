#include "ExternalVdrAdapter.h"

ExternalVdrAdapter::ExternalVdrAdapter(VdrConfig config)
    : config_(config)
{
}

VdrStatus ExternalVdrAdapter::getStatus() const
{
    VdrStatus status;
    status.enabled = config_.enabled;
    status.mode = config_.mode;
    status.host = config_.host;
    status.port = config_.port;
    status.state = config_.enabled ? "configured" : "disabled";

    return status;
}
