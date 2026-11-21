#ifndef VDR_SUITE_BRIDGE_EPGSEARCH_NATIVE_CAS_CONTROL_H
#define VDR_SUITE_BRIDGE_EPGSEARCH_NATIVE_CAS_CONTROL_H

#include "SuiteBridgeControlPlaneProtocol.h"
#include "suitebridge_command_result.h"
#include "suitebridge_epgsearch_native_cas.h"

#include <string>

// Strictly local SO_PEERCRED-protected control-plane adapter.
// There is intentionally no VDR SVDRP / HTTP SearchTimer write command.
// The caller must validate backend scope and acquire a durable operation CAS
// before invoking SearchTimerCasExecute, and never retry uncertain outcomes.
class SuiteBridgeEpgsearchCasControl final {
public:
    SuiteBridgeCommandResult Handle(
        vdrsuite::agent::control::Operation operation,
        const std::string& payload) const;
private:
    SuiteBridgeEpgsearchNativeCas native_;
};

#endif
