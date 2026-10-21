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
):
    if required not in runtime:
        raise SystemExit("missing safe delayed MODIFY dispatcher gate: " + required)
for required in (
    "if (op.deadline != 0 && op.deadline <= now)",
    "commands.findAssignmentForOperation(",
    "MutationOperationState::failedBeforeDispatch",
    "native-timer-modify:expired-before-assignment",
):
    assert required in runtime, "missing expired-unassigned recovery fence: " + required
summary = (ROOT/"core/daemon/include/NativeTimerModifyProductiveRuntime.h").read_text()
assert "std::size_t expired = 0;" in summary
# Legacy MODIFY results without command-bound evidence may not hold dispatching
# forever. Revision-CAS quarantine is outcome_unknown and never a blind retry.
for required in (
    "durableResult->resultEvidence.empty()",
    "op.state == MutationOperationState::dispatching",
    "MutationOperationState::outcomeUnknown",
    "native-timer-modify:missing-result-evidence",
    "if (quarantined.ok()) ++summary.quarantined;",
):
    assert required in runtime, "missing evidence-free MODIFY quarantine: " + required
assert "std::size_t quarantined = 0;" in summary
assert "if (expired.ok()) ++summary.expired;" in runtime
assert "if (commands.findAssignmentForOperation(" in runtime
assert "b.lastObservedAt < now - 300" not in runtime, (
    "accepted MODIFY must not deadlock on its own frozen observation")
print("phase64 native Timer modify assignment guard passed")
