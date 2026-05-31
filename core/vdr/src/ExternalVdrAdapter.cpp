#include "ExternalVdrAdapter.h"

ExternalVdrAdapter::ExternalVdrAdapter()
    : ExternalVdrAdapter(VdrConfig())
{
}

ExternalVdrAdapter::ExternalVdrAdapter(const VdrConfig& config)
    : enabled_(config.enabled),
      mode_(config.mode),
      host_(config.host),
      port_(config.port),
      state_("configured")
{
}

VdrStatus ExternalVdrAdapter::getStatus() const
{
    return VdrStatus{
        enabled_,
        mode_,
        host_,
        port_,
        state_
    };
}
