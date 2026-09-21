#!/usr/bin/env python3
"""Static guard for the real Phase-63 Backend Agent acceptance harness."""

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools/phase63-runtime-acceptance/backend-agent-foundation.sh"
UPGRADE_RUNNER = ROOT / "tools/phase63-runtime-acceptance/backend-health-ingestion.sh"
MAKEFILE = ROOT / "mk/phase63-runtime-acceptance.mk"
RUNBOOK = ROOT / "docs/development/phase-63-backend-agent-runtime-acceptance-runbook.md"
SECRET_SCANNER = ROOT / "tools/check_phase63_runtime_evidence_secrets.py"
OBSERVATION_EXERCISER = (
    ROOT / "tools/phase63-runtime-acceptance/exercise_backend_health_observation.py"
)
CHANNEL_RUNNER = ROOT / "tools/phase63-runtime-acceptance/channel-observation-ingestion.sh"
CHANNEL_OBSERVATION_EXERCISER = (
    ROOT / "tools/phase63-runtime-acceptance/exercise_channel_observation.py"
)

failures: list[str] = []
for path in (
    RUNNER, UPGRADE_RUNNER, CHANNEL_RUNNER, MAKEFILE, RUNBOOK, SECRET_SCANNER,
    OBSERVATION_EXERCISER, CHANNEL_OBSERVATION_EXERCISER,
):
    if not path.is_file():
        failures.append(f"missing Phase-63 runtime acceptance file: {path.relative_to(ROOT)}")

if failures:
    for failure in failures:
        print(failure, file=sys.stderr)
    raise SystemExit(1)

runner = RUNNER.read_text(encoding="utf-8")
upgrade_runner = UPGRADE_RUNNER.read_text(encoding="utf-8")
channel_runner = CHANNEL_RUNNER.read_text(encoding="utf-8")
makefile = MAKEFILE.read_text(encoding="utf-8")
runbook = RUNBOOK.read_text(encoding="utf-8")

required_runner = [
    "set -euo pipefail",
    "cleanup_failed_acceptance",
    "DELETE FROM backend_agents WHERE backend_id",
    "DELETE FROM backend_agent_observation_receipts WHERE backend_id",
    "DELETE FROM backend_agent_observation_cursors WHERE backend_id",
    "root_required",
    "expected_branch_required",
    "expected_head_required",
    "worktree_not_clean",
    "installed_candidate_mismatch_",
    "control_plane_url_must_be_https",
    "control_plane_tls_probe_failed",
    "control_plane_api_route_not_found",
    "backend_agent_history_already_present_for_backend",
    "wait_for_state online",
    "wait_for_state stale",
    "wait_for_state offline",
    "--rotate-credential",
    "--revoke --reason runtime-acceptance-replacement",
    "revoked_agent_reconnected",
    "replacement_agent_identity_not_distinct",
    "vdr_native_state_changed",
    "evidence-secret-scan.txt",
    "secret_like_material_found_in_evidence_logs",
    "PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=PASS",
    "PHASE_63_BACKEND_HEALTH_INGESTION_RUNTIME_ACCEPTANCE=PASS",
    "backendHealthObservation",
    "OBSERVATION_REPLACEMENT_CURSOR=yes",
    "BACKEND_HEALTH_OBSERVATION_REPLAY=yes",
    "BACKEND_HEALTH_OBSERVATION_GAP_RESYNC=yes",
    "OBSERVATION_CURSOR_RESTART_PERSISTED=yes",
    "observation-replay-gap.log",
    'systemctl restart "$DAEMON_SERVICE"',
]
for marker in required_runner:
    if marker not in runner:
        failures.append(f"runtime acceptance guard missing: {marker}")

for forbidden in ["curl -k", "--insecure", "set +e", "cat \"$CONFIG_PATH\""]:
    if forbidden in runner:
        failures.append(f"runtime acceptance contains forbidden unsafe pattern: {forbidden}")

required_upgrade_runner = [
    "set -euo pipefail",
    "existing_agent_identity_required",
    "agent_not_active_before_acceptance",
    "installed_candidate_mismatch_",
    "wait_for_online_observation",
    "wait_for_observation_advance",
    "agent_identity_changed_after_restart",
    "agent_identity_changed_after_gap",
    "BACKEND_HEALTH_OBSERVATION_REPLAY=PASS",
    "BACKEND_HEALTH_OBSERVATION_GAP_RESYNC=PASS",
    "OBSERVATION_CURSOR_RESTART_PERSISTED=yes",
    "EXISTING_AGENT_IDENTITY_PRESERVED=yes",
    "PHASE_63_BACKEND_HEALTH_INGESTION_UPGRADE_ACCEPTANCE=PASS",
    "evidence-secret-scan.txt",
    "vdr_native_state_changed",
]
for marker in required_upgrade_runner:
    if marker not in upgrade_runner:
        failures.append(f"upgrade runtime acceptance guard missing: {marker}")

for forbidden in [
    "DELETE FROM backend_agents",
    "--revoke",
    '"$ENROLL_BINARY" --database',
    "curl -k",
    "--insecure",
    "set +e",
]:
    if forbidden in upgrade_runner:
        failures.append(
            f"upgrade runtime acceptance contains destructive or unsafe pattern: {forbidden}"
        )


required_channel_runner = [
    "PHASE_63_CHANNEL_OBSERVATION_UPGRADE_ACCEPTANCE=PASS",
    "fixture_must_not_be_native_channels_conf",
    "CHANNELS_CONF_PATH",
    "channels-conf",
    "CHANNEL_BASELINE=yes",
    "CHANNEL_FIXTURE_TRANSITION=yes",
    "CHANNEL_OBSERVATION_REPLAY=PASS",
    "CHANNEL_OBSERVATION_GAP_RESYNC=PASS",
    "CHANNEL_OBSERVATION_FACTS_UNCHANGED=PASS",
    "CHANNEL_CURSOR_RESTART_PERSISTED=yes",
    "CHANNEL_RECOVERY_AFTER_RESYNC=yes",
    "ORIGINAL_CONFIGURATION_RESTORED=yes",
    "EXISTING_AGENT_IDENTITY_PRESERVED=yes",
    "CREDENTIAL_GENERATION_PRESERVED=yes",
    "vdr-state.before",
    "vdr-state.after",
    "vdr-readonly.before.log",
    "vdr-readonly.after.log",
    "evidence-secret-scan.txt",
    "installed_candidate_mismatch_",
    "candidate_binary_build_failed",
    "make daemon backend-agent backend-agent-enrollment backend-agent-admin",
    '|| fail "initial_channel_observation_not_observed"',
    '|| fail "changed_channel_snapshot_not_observed"',
    '|| fail "new_channel_observation_lineage_not_observed"',
    '|| fail "channel_recovery_after_resync_not_observed"',
    "channel-replay-gap.log",
    'systemctl restart "$DAEMON_SERVICE"',
]
for marker in required_channel_runner:
    if marker not in channel_runner:
        failures.append(f"Channel runtime acceptance guard missing: {marker}")

for forbidden in [
    "DELETE FROM backend_agents",
    "--revoke",
    '"$ENROLL_BINARY" --database',
    "curl -k",
    "--insecure",
    "set -e",
    "set -u",
    "set -o pipefail",
    'cat "$CONFIG_PATH"',
]:
    if forbidden in channel_runner:
        failures.append(
            f"Channel runtime acceptance contains destructive or unsafe pattern: {forbidden}"
        )

required_make = [
    "test-phase63-runtime-acceptance-harness",
    "phase63-backend-agent-runtime-acceptance",
    "phase63-backend-health-ingestion-runtime-acceptance",
    "PHASE63_INGESTION_ACCEPTANCE_RUNNER",
    "PHASE63_EXPECTED_BRANCH",
    "PHASE63_EXPECTED_HEAD",
    "PHASE63_CONTROL_PLANE_URL",
    "PHASE63_EVIDENCE_DIR",
    "PHASE63_OBSERVATION_EXERCISER",
    "PHASE63_CHANNEL_ACCEPTANCE_RUNNER",
    "PHASE63_CHANNEL_OBSERVATION_EXERCISER",
    "phase63-channel-observation-runtime-acceptance",
    "PHASE63_CHANNELS_CONF_SOURCE",
    "--self-test",
]
for marker in required_make:
    if marker not in makefile:
        failures.append(f"runtime acceptance Make contract missing: {marker}")


helper_test = subprocess.run(
    [sys.executable, str(OBSERVATION_EXERCISER), "--self-test"],
    cwd=ROOT,
    text=True,
    capture_output=True,
    check=False,
)
if helper_test.returncode != 0:
    failures.append(
        "backend-health observation acceptance helper self-test failed: "
        + (helper_test.stderr.strip() or helper_test.stdout.strip() or "unknown error")
    )


channel_helper_test = subprocess.run(
    [sys.executable, str(CHANNEL_OBSERVATION_EXERCISER), "--self-test"],
    cwd=ROOT,
    text=True,
    capture_output=True,
    check=False,
)
if channel_helper_test.returncode != 0:
    failures.append(
        "Channel observation acceptance helper self-test failed: "
        + (
            channel_helper_test.stderr.strip()
            or channel_helper_test.stdout.strip()
            or "unknown error"
        )
    )

scanner_test = subprocess.run(
    [sys.executable, str(SECRET_SCANNER), "--self-test"],
    cwd=ROOT,
    text=True,
    capture_output=True,
    check=False,
)
if scanner_test.returncode != 0:
    failures.append(
        "runtime evidence secret scanner self-test failed: "
        + (scanner_test.stderr.strip() or scanner_test.stdout.strip() or "unknown error")
    )

for marker in [
    "exact PR head",
    "VDR-native state",
    "revoked Agent",
    "replacement Agent",
    "PHASE_63_BACKEND_AGENT_RUNTIME_ACCEPTANCE=PASS",
    "equivalent replay",
    "sequence gap",
    "daemon restart",
    "upgrade-safe",
    "existing active Agent",
    "PHASE_63_BACKEND_HEALTH_INGESTION_UPGRADE_ACCEPTANCE=PASS",
    "Channel observation",
    "root-controlled fixture",
    "PHASE_63_CHANNEL_OBSERVATION_UPGRADE_ACCEPTANCE=PASS",
]:
    if marker not in runbook:
        failures.append(f"runtime acceptance runbook missing: {marker}")

if failures:
    for failure in failures:
        print(failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-63 runtime acceptance guard passed")
