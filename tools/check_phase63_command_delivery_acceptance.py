#!/usr/bin/env python3
"""Static guard for the bounded Phase-63 command-delivery acceptance runner."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools/phase63-runtime-acceptance/command-delivery.sh"
MAKEFILE = ROOT / "mk/phase63-runtime-acceptance.mk"
RUNTIME_GUARD = ROOT / "tools/check_phase63_command_delivery_runtime.py"

failures: list[str] = []


def require(path: Path, markers: list[str]) -> str:
    if not path.is_file():
        failures.append(f"missing file: {path.relative_to(ROOT)}")
        return ""
    text = path.read_text(encoding="utf-8")
    for marker in markers:
        if marker not in text:
            failures.append(f"{path.relative_to(ROOT)} misses marker: {marker}")
    return text


runner = require(
    RUNNER,
    [
        "PHASE_63_COMMAND_DELIVERY_UPGRADE_ACCEPTANCE=PASS",
        "candidate_binary_build_failed",
        "make daemon backend-agent backend-agent-enrollment backend-agent-admin backend-agent-command-admin",
        "installed_candidate_mismatch_",
        "COMMAND_TYPES=probe.noop",
        "command_runtime_already_enabled",
        "command_capability_or_assignment_not_available",
        "BASELINE_COMMAND_COMPLETED=yes",
        "COMMAND_REPLAY=yes",
        "LOST_RECEIPT_RESPONSE_RECOVERED=yes",
        "LOST_RESULT_RESPONSE_RECOVERED=yes",
        "DAEMON_RESTART_PERSISTED=yes",
        "AGENT_RESTART_RECOVERED=yes",
        "STALE_GENERATION_COMMAND_NOT_REPLAYED=yes",
        "acknowledged_stale_command_state_not_retired",
        "EXISTING_AGENT_IDENTITY_PRESERVED=yes",
        "CREDENTIAL_GENERATION_PRESERVED=yes",
        "VDR_NATIVE_STATE_UNCHANGED=yes",
        "ORIGINAL_CONFIGURATION_RESTORED=yes",
        "restore_runtime strict",
        "evidence-secret-scan.txt",
        "check_phase63_runtime_evidence_secrets.py",
        "vdr-state.before",
        "vdr-state.after",
        "systemctl restart \"$DAEMON_SERVICE\"",
        "systemctl restart \"$AGENT_SERVICE\"",
        "--arm-lost-receipt-response",
        "--arm-lost-result-response",
        "--replay \"$BASELINE_ID\"",
    ],
)

for forbidden in [
    "set -e",
    "set -u",
    "set -o pipefail",
    "set +e",
    "curl -k",
    "--insecure",
    "sqlite3 ",
    "DELETE FROM ",
    "UPDATE backend_agent_",
    'cat "$CONFIG_PATH"',
    "timer.create",
    "recording.delete",
    "searchtimer.create",
]:
    if forbidden in runner:
        failures.append(f"command acceptance runner contains forbidden pattern: {forbidden}")

makefile = require(
    MAKEFILE,
    [
        "PHASE63_COMMAND_ACCEPTANCE_RUNNER",
        "tools/phase63-runtime-acceptance/command-delivery.sh",
        "bash -n \"$(PHASE63_COMMAND_ACCEPTANCE_RUNNER)\"",
        "phase63-command-delivery-runtime-acceptance",
        "PHASE63_AGENT_CONFIG_PATH",
        "PHASE63_DAEMON_SERVICE",
        "PHASE63_AGENT_SERVICE",
        "PHASE63_VDR_SERVICE",
    ],
)

require(
    RUNTIME_GUARD,
    [
        "replay counters expose durable acceptance evidence",
        "acknowledged stale generation state retires; pending state stays fenced",
    ],
)

if failures:
    print("Phase-63 command delivery acceptance guard failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-63 command delivery acceptance guard passed")
print("Command type: probe.noop only")
print("Native VDR mutation: absent")
print("Manual SQLite inspection: absent")
