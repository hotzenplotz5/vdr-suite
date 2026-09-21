#!/usr/bin/env python3
"""Static guard for the Phase-63 fenced native operation contract."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "docs/development/phase-63-fenced-native-operation.md"
CLOSEOUT = ROOT / "docs/development/phase-63-slice-3-closeout.md"
COMMAND_CONTRACT = ROOT / "docs/development/phase-63-command-delivery.md"
COMMAND_RUNTIME = ROOT / "docs/development/phase-63-command-delivery-runtime.md"
ADR_MUTATION = ROOT / "docs/adr/ADR-0042-safe-mutation-revision-idempotency-contract.md"
ADR_JOB = ROOT / "docs/adr/ADR-0043-job-claim-retry-saga-execution-model.md"
PLUGIN_ADR = (
    ROOT
    / "vdr-plugin-suite-bridge/docs/ADR-0001-plugin-role-and-native-integration-strategy.md"
)
PLUGIN_ROADMAP = ROOT / "vdr-plugin-suite-bridge/docs/ROADMAP.md"
MAKE_FRAGMENT = ROOT / "mk/phase63-runtime-acceptance.mk"
PLUGIN_SVDRP = ROOT / "vdr-plugin-suite-bridge/suitebridge_svdrp.cpp"
PLUGIN_CAPABILITIES = ROOT / "vdr-plugin-suite-bridge/suitebridge_capabilities.cpp"
LOCAL_TRANSPORT = ROOT / "core/agent/include/ISuiteBridgeLocalTransport.h"
COMMAND_CLIENT = ROOT / "core/agent/src/BackendAgentCommandClient.cpp"
AGENT_CONFIG = ROOT / "packaging/systemd/backend-agent.conf"
RUNTIME_GUARD = ROOT / "tools/check_phase63_fenced_native_operation_runtime.py"

BASE_COMMIT = "271254a5e5baf83f4a32e974da3d6bec7e33064b"
BASE_TREE = "4b4b3c89498bf15397d27dffbf1cbcb114673825"

failures: list[str] = []


def read(path: Path) -> str:
    if not path.is_file():
        failures.append(f"missing required file: {path.relative_to(ROOT)}")
        return ""
    return path.read_text(encoding="utf-8")


def normalized(value: str) -> str:
    return " ".join(value.casefold().split())


def require(path: Path, markers: list[str]) -> None:
    text = normalized(read(path))
    for marker in markers:
        if normalized(marker) not in text:
            failures.append(
                f"{path.relative_to(ROOT)} misses required marker: {marker}"
            )


require(
    CONTRACT,
    [
        "contract-only slice",
        BASE_COMMIT,
        BASE_TREE,
        "Phase 63 Slice 4",
        "fenced native operation",
        "vdr.native.probe",
        "payloadVersion = 1",
        "verificationPolicy = readback_required",
        "sideEffectClass = none",
        "Control Plane remains authoritative",
        "operationId",
        "jobId",
        "attemptId",
        "claimEpoch",
        "commandId",
        "requestFingerprint",
        "agentInstanceId",
        "backendGeneration",
        "pluginInstanceEpoch",
        "nativeExecutionSequence",
        "mutations=disabled",
        "localProviderKind = suitebridge",
        "Provider ownership and selection remain a separate later Phase-63 slice",
        "no fallback to free-form SVDRP",
        "public C++ boundary must not accept free command text",
        "reserve an epoch-scoped bounded receipt entry",
        "same `commandId` with a different fingerprint",
        "stored receipt for an identical duplicate",
        "not sufficient idempotency for a production mutation",
        "not_started",
        "starting",
        "accepted_by_executor",
        "effect_reported",
        "No local call occurs before `starting` is durable",
        "readback_required",
        "waiting_reconciliation",
        "outcome_unknown",
        "do not blindly execute",
        "pre-dispatch AccountabilityEvent",
        "no public Agent, provider, plugin or SVDRP endpoint",
        "does not authorize a production VDR mutation",
        "separate bounded Draft runtime PR",
        "no real yaVDR acceptance",
        "manual SQLite inspection",
        "Phase 64",
    ],
)

require(
    CLOSEOUT,
    [
        "Phase 63 Slice 3",
        "completed, accepted on the real yaVDR host and merged",
        "PR #142",
        "377165c7dff165542a2ab171bc94c97574e044e8",
        "2f613b1b1a889d104b78b96f2d7d638724c315c4",
        "PR #143",
        "e1abe3a9bbcb821398d39249e2d6ada9d8977c3a",
        BASE_COMMIT,
        BASE_TREE,
        "run number: 7300",
        "run number: 7325",
        "PHASE_63_COMMAND_DELIVERY_UPGRADE_ACCEPTANCE=PASS",
        "BASELINE_COMMAND_COMPLETED=yes",
        "COMMAND_REPLAY=yes",
        "LOST_RECEIPT_RESPONSE_RECOVERED=yes",
        "LOST_RESULT_RESPONSE_RECOVERED=yes",
        "DAEMON_RESTART_PERSISTED=yes",
        "AGENT_RESTART_RECOVERED=yes",
        "STALE_GENERATION_COMMAND_NOT_REPLAYED=yes",
        "EXISTING_AGENT_IDENTITY_PRESERVED=yes",
        "CREDENTIAL_GENERATION_PRESERVED=yes",
        "VDR_NATIVE_STATE_UNCHANGED=yes",
        "ORIGINAL_CONFIGURATION_RESTORED=yes",
        "no manual SQLite inspection",
        "Phase 63 Slice 4",
        "vdr.native.probe",
    ],
)

require(
    COMMAND_CONTRACT,
    [
        "durable Agent command delivery",
        "Agent durable inbox",
        "Result outbox",
        "starting",
        "outcome_unknown",
        "separate command-specific contract",
    ],
)

require(
    COMMAND_RUNTIME,
    [
        "probe.noop",
        "non-mutating",
        "starting",
        "stores the result before transport",
        "no native VDR mutation",
    ],
)

require(
    ADR_MUTATION,
    [
        "backendGeneration",
        "expectedRevision",
        "idempotencyKey",
        "readback_required",
        "outcome_unknown",
        "reconciliation",
    ],
)

require(
    ADR_JOB,
    [
        "claimEpoch",
        "Dispatch Boundary",
        "starting",
        "accepted_by_executor",
        "effect_reported",
        "waiting_reconciliation",
    ],
)

require(
    PLUGIN_ADR,
    [
        "narrowly typed native operations",
        "Backend Agent",
        "Control Plane",
        "Typed Native Action Strategy",
        "stable typed request schema",
        "authoritative readback or reconciliation",
        "mutations=disabled",
        "universal plugin-service or SVDRP tunnel",
    ],
)

require(
    PLUGIN_ROADMAP,
    [
        "mutations            disabled",
        "commands             CAPS and SNAP only",
        "SB.16 Typed native actions",
        "candidate",
        "after mutation foundation",
    ],
)

require(
    MAKE_FRAGMENT,
    [
        "test-phase63-fenced-native-operation-contract",
        "tools/check_phase63_fenced_native_operation_contract.py",
    ],
)

runtime_present = RUNTIME_GUARD.is_file()
if runtime_present:
    require(
        RUNTIME_GUARD,
        [
            "vdr.native.probe only",
            "sideEffectClass = none",
            "mutations=disabled",
            "native execution sequence",
            "plugin instance epoch",
            "no production VDR mutation",
        ],
    )
else:
    for path, forbidden_markers in [
        (
            PLUGIN_SVDRP,
            [
                "vdr.native.probe",
                "NPROBE",
                "NativeProbe",
                "nativeExecutionSequence",
            ],
        ),
        (
            PLUGIN_CAPABILITIES,
            [
                "vdr.native.probe",
                "native_operation_probe",
                "nativeOperationSchema",
            ],
        ),
        (
            LOCAL_TRANSPORT,
            [
                "NativeProbe",
                "nativeProbe",
                "vdr.native.probe",
            ],
        ),
        (
            COMMAND_CLIENT,
            [
                "vdr.native.probe",
                "executeNativeProbe",
                "pluginInstanceEpoch",
            ],
        ),
        (
            AGENT_CONFIG,
            [
                "vdr.native.probe",
                "NATIVE_PROBE",
            ],
        ),
    ]:
        candidate = read(path)
        for marker in forbidden_markers:
            if marker in candidate:
                failures.append(
                    f"premature fenced-native runtime marker in "
                    f"{path.relative_to(ROOT)}: {marker}"
                )

contract_text = read(CONTRACT)
for forbidden in [
    "free-form SVDRP is allowed",
    "generic plugin service payload is allowed",
    "timeout proves no native execution",
    "retry immediately after outcome_unknown",
    "the probe establishes provider ownership",
    "browser calls SuiteBridge directly",
    "manual SQLite inspection is required",
]:
    if forbidden in contract_text:
        failures.append(
            f"fenced native operation contract contains forbidden scope: {forbidden}"
        )

if failures:
    print("Phase-63 fenced native operation contract check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-63 fenced native operation contract check passed")
print(f"Merged command-delivery base: {BASE_COMMIT}")
print(f"Merged command-delivery tree: {BASE_TREE}")
print("Next bounded command type: vdr.native.probe")
print(
    "Runtime implementation: bounded and guarded"
    if runtime_present
    else "Runtime implementation: separate bounded Draft PR"
)
