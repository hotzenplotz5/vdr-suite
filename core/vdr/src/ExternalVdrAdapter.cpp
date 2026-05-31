#include "ExternalVdrAdapter.h"

ExternalVdrAdapter::ExternalVdrAdapter()
    : enabled_(true),
      mode_("external"),
      host_("127.0.0.1"),
      port_(6419),
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
