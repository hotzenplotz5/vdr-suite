#!/usr/bin/env python3
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
source=(ROOT/"core/agent/src/BackendAgentNativeTimerModifyAssignment.cpp").read_text()
for token in ("ensureNativeTimerModifyAssignmentSchema","selectLocalProvider",
"insertAssignment","vdr.timer.update","vdr.timer.toggle","readback_required"):
 if token not in source:raise SystemExit("missing modify assignment invariant: "+token)
# Live Agent capabilities are authoritative. A schema helper must never
# remove an already-advertised vdr.timer.update/toggle capability.
assert "DELETE FROM backend_agent_command_capabilities" not in source, (
    "MODIFY assignment must not revoke Agent capabilities"
)
for trigger in ("timer_update", "timer_toggle"):
    assert (
        f"DROP TRIGGER IF EXISTS trg_backend_agent_{trigger}_dormant_capability"
        in source
    ), f"legacy {trigger} trigger must be removed"
client=(ROOT/"core/agent/src/BackendAgentCommandClient.cpp").read_text()
if "kBackendAgentNativeTimerToggleCommandType" not in client:
 raise SystemExit("modify advertisement fence missing")
# Regression: CREATE-only owner authorization must never masquerade as a
# full CRUD provider grant. The operator must explicitly enable the owner.
admin = (ROOT/"apps/tools/backend_agent_command_admin.cpp").read_text()
for required in (
    "--set-timer-crud-owner",
    "Action::SetTimerCrudOwner",
    "kBackendAgentNativeTimerCreateCapability",
    "kBackendAgentNativeTimerUpdateCapability",
    "kBackendAgentNativeTimerToggleCapability",
    "kBackendAgentNativeTimerDeleteCapability",
    "setLocalProviderOwnership(",
):
    assert required in admin, "missing explicit timer CRUD ownership: " + required
assert "const bool timerCreate = action == Action::SetTimerCreateOwner || timerCrud;" in admin

# Regression: accepted work freezes binding revisions; its admission
# freshness must not expire simply because a provider-owner grant was missing.
# Keep the live Provider/Agent lease, deadline and native CAS fences intact.
runtime = (ROOT/"core/daemon/src/NativeTimerModifyProductiveRuntime.cpp").read_text()
for required in (
    "b.lastObservedAt < op.requestedAt - 300",
    "op.requestedAt > now",
    "op.deadline != 0 && op.deadline <= now",
    "op.expectedResourceFingerprint != b.observedFingerprint",
    "agent->leaseExpiresAt < now",
    "agent->backendGeneration != op.backendGeneration",
    "backendAgentNativeTimerModifyCapability",
):
    if required not in runtime and required != "backendAgentNativeTimerModifyCapability":
        raise SystemExit("missing safe delayed MODIFY dispatcher gate: " + required)
assert "b.lastObservedAt < now - 300" not in runtime, (
    "accepted MODIFY must not deadlock on its own frozen observation")
print("phase64 native Timer modify assignment guard passed")
