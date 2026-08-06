#!/usr/bin/env python3
"""Static guard for the Phase-63 durable Agent command delivery contract."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "docs/development/phase-63-command-delivery.md"
CLOSEOUT = ROOT / "docs/development/phase-63-slice-2-closeout.md"
OBSERVATION_CONTRACT = ROOT / "docs/development/phase-63-observation-ingestion.md"
ADR_MUTATION = ROOT / "docs/adr/ADR-0042-safe-mutation-revision-idempotency-contract.md"
ADR_JOB = ROOT / "docs/adr/ADR-0043-job-claim-retry-saga-execution-model.md"
TARGET_ARCHITECTURE = ROOT / "docs/architecture/target-platform-architecture.md"
HTTP_SERVER = ROOT / "core/agent/src/BackendAgentHttpServer.cpp"
CLIENT_HEADER = ROOT / "core/agent/include/BackendAgentClient.h"
SCHEMA = ROOT / "database/schema/vdr-suite.sql"
AGENT_CONFIG = ROOT / "packaging/systemd/backend-agent.conf"
MAKE_FRAGMENT = ROOT / "mk/phase63-runtime-acceptance.mk"
RUNTIME_GUARD = ROOT / "tools/check_phase63_command_delivery_runtime.py"

BASE_COMMIT = "39ed86fc3a425697f738f8f555394d54e4e1a684"
BASE_TREE = "e03bb84951cef7ec5f6b2f338ba456116cd766a2"

failures: list[str] = []


def read(path: Path) -> str:
    if not path.is_file():
        failures.append(f"missing required file: {path.relative_to(ROOT)}")
        return ""
    return path.read_text(encoding="utf-8")


def require(path: Path, markers: list[str]) -> None:
    text = read(path)
    folded = text.casefold()
    for marker in markers:
        if marker.casefold() not in folded:
            failures.append(
                f"{path.relative_to(ROOT)} misses required marker: {marker}"
            )


require(
    CONTRACT,
    [
        "contract-only slice",
        BASE_COMMIT,
        BASE_TREE,
        "Phase 63 Slice 3",
        "durable Agent command delivery",
        "Control Plane authoritative",
        "operationId",
        "jobId",
        "attemptId",
        "claimEpoch",
        "commandId",
        "requestFingerprint",
        "agentInstanceId",
        "backendGeneration",
        "payloadVersion",
        "expectedRevision",
        "verificationPolicy",
        "`claimToken` is not copied",
        "outbound-only",
        "no public Agent/provider endpoint",
        "Agent durable inbox",
        "durable receipt",
        "conflicting duplicate",
        "not_started",
        "starting",
        "accepted_by_executor",
        "effect_reported",
        "Result outbox",
        "result replay is idempotent",
        "outcome_unknown",
        "waiting_reconciliation",
        "reconnect order",
        "pre-dispatch AccountabilityEvent",
        "Repository classes own SQLite",
        "no native VDR mutation",
        "separate command-specific contract",
        "installation or real yaVDR acceptance",
        "separate bounded Draft runtime PR",
        "Phase 64",
        "manual SQLite",
    ],
)

require(
    CLOSEOUT,
    [
        "Phase 63 Slice 2",
        "completed, accepted on the real yaVDR host and merged",
        "PR #138",
        "24b1d7938ddaa15834a8da6323a270761868f4ba",
        "PR #139",
        "37df59552fd6d2f739c580dc9b472416f0bf5a12",
        "PHASE_63_BACKEND_HEALTH_INGESTION_UPGRADE_ACCEPTANCE=PASS",
        "PR #140",
        "2567407e6e1c6d098804f887875e7f3cbf9cba60",
        "PR #141",
        BASE_COMMIT,
        "PHASE_63_CHANNEL_OBSERVATION_UPGRADE_ACCEPTANCE=PASS",
        "INITIAL_CHANNEL_FACT_COUNT=343",
        "EXISTING_AGENT_IDENTITY_PRESERVED=yes",
        "CREDENTIAL_GENERATION_PRESERVED=yes",
        "VDR_NATIVE_STATE_UNCHANGED=yes",
        "ORIGINAL_CONFIGURATION_RESTORED=yes",
        "no manual SQLite inspection",
        "Phase 63 Slice 3",
    ],
)

require(
    OBSERVATION_CONTRACT,
    [
        "vdr-suite-agent/1",
        "completeSnapshot",
        "changeBatch",
        "resync-required",
        "command inbox",
        "VDR-native execution",
    ],
)

require(
    ADR_MUTATION,
    [
        "operationId",
        "idempotencyKey",
        "backendGeneration",
        "expectedRevision",
        "outcome_unknown",
        "reconciliation",
    ],
)

require(
    ADR_JOB,
    [
        "claimEpoch",
        "commandId",
        "Agent inbox and result-outbox reconciliation",
        "Backend Agent Command Contract",
        "Offline Agent and Reconnect Model",
        "starting",
        "waiting_reconciliation",
    ],
)

require(
    TARGET_ARCHITECTURE,
    [
        "commands/results and native execution [later Phase 63]",
        "safe mutation and durable execution target",
        "fenced Agent/native command",
        "authoritative readback and verification",
    ],
)

require(
    MAKE_FRAGMENT,
    [
        "test-phase63-command-delivery-contract",
        "tools/check_phase63_command_delivery_contract.py",
    ],
)

runtime_present = RUNTIME_GUARD.is_file()
if runtime_present:
    require(
        RUNTIME_GUARD,
        [
            "probe.noop only",
            "Native VDR mutation: absent",
            "command_capability_required",
            "equivalent replay preserves the durable receipt",
            "parameterized capability cleanup",
        ],
    )
else:
    for path, forbidden_markers in [
        (
            HTTP_SERVER,
            [
                "/api/agent/v1/commands",
                "handleAgentCommand",
                "handleCommandReceipt",
                "handleCommandResult",
            ],
        ),
        (
            CLIENT_HEADER,
            [
                "commandInbox",
                "resultOutbox",
                "pendingCommand",
                "publishCommandResult",
            ],
        ),
        (
            SCHEMA,
            [
                "backend_agent_commands",
                "backend_agent_command_receipts",
                "backend_agent_command_results",
                "backend_agent_command_inbox",
                "backend_agent_result_outbox",
            ],
        ),
        (
            AGENT_CONFIG,
            [
                "COMMAND_TYPES=",
                "COMMAND_POLL_INTERVAL=",
            ],
        ),
    ]:
        candidate = read(path)
        for marker in forbidden_markers:
            if marker in candidate:
                failures.append(
                    f"premature command runtime marker in {path.relative_to(ROOT)}: "
                    f"{marker}"
                )

contract_text = read(CONTRACT)
for forbidden in [
    "browser credentials are accepted for Agent commands",
    "Agent chooses the backend generation",
    "timeout means the command failed",
    "retry the mutation immediately after outcome_unknown",
    "claimToken is included in the Agent payload",
    "generic command payload authorizes all VDR mutations",
    "public Agent URL is required",
    "manual SQLite inspection is required",
]:
    if forbidden in contract_text:
        failures.append(f"command delivery contract contains forbidden scope: {forbidden}")

if failures:
    print("Phase-63 command delivery contract check failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-63 command delivery contract check passed")
print(f"Merged read-only ingestion base: {BASE_COMMIT}")
print(f"Merged read-only ingestion tree: {BASE_TREE}")
print("Next bounded slice: durable Agent command delivery")
print("Runtime implementation: bounded and guarded" if runtime_present else "Runtime implementation: separate bounded Draft PR")
