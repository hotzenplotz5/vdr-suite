#!/usr/bin/env python3
"""Static guard for the real Phase-63 Backend Agent acceptance harness."""

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools/phase63-runtime-acceptance/backend-agent-foundation.sh"
MAKEFILE = ROOT / "mk/phase63-runtime-acceptance.mk"
RUNBOOK = ROOT / "docs/development/phase-63-backend-agent-runtime-acceptance-runbook.md"
SECRET_SCANNER = ROOT / "tools/check_phase63_runtime_evidence_secrets.py"

failures: list[str] = []
for path in (RUNNER, MAKEFILE, RUNBOOK, SECRET_SCANNER):
    if not path.is_file():
        failures.append(f"missing Phase-63 runtime acceptance file: {path.relative_to(ROOT)}")

if failures:
    for failure in failures:
        print(failure, file=sys.stderr)
    raise SystemExit(1)

runner = RUNNER.read_text(encoding="utf-8")
makefile = MAKEFILE.read_text(encoding="utf-8")
runbook = RUNBOOK.read_text(encoding="utf-8")

required_runner = [
    "set -euo pipefail",
    "cleanup_failed_acceptance",
    "DELETE FROM backend_agents WHERE backend_id",
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
]
for marker in required_runner:
    if marker not in runner:
        failures.append(f"runtime acceptance guard missing: {marker}")

for forbidden in ["curl -k", "--insecure", "set +e", "cat \"$CONFIG_PATH\""]:
    if forbidden in runner:
        failures.append(f"runtime acceptance contains forbidden unsafe pattern: {forbidden}")

required_make = [
    "test-phase63-runtime-acceptance-harness",
    "phase63-backend-agent-runtime-acceptance",
    "PHASE63_EXPECTED_BRANCH",
    "PHASE63_EXPECTED_HEAD",
    "PHASE63_CONTROL_PLANE_URL",
    "PHASE63_EVIDENCE_DIR",
]
for marker in required_make:
    if marker not in makefile:
        failures.append(f"runtime acceptance Make contract missing: {marker}")


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
]:
    if marker not in runbook:
        failures.append(f"runtime acceptance runbook missing: {marker}")

if failures:
    for failure in failures:
        print(failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-63 runtime acceptance guard passed")
