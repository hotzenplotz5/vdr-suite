#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RUNNER = ROOT / "tools/run_phase64_reassignment_failover_acceptance.sh"
BASE_RUNNER = ROOT / "tools/run_phase64_managed_timer_fulfillment_acceptance.sh"
FRAGMENT = ROOT / "mk/phase64-reassignment-failover-acceptance.mk"
MAKEFILE = ROOT / "Makefile"

for path in (RUNNER, BASE_RUNNER, FRAGMENT, MAKEFILE):
    if not path.is_file():
        raise SystemExit(f"missing reassignment acceptance file: {path.relative_to(ROOT)}")

runner = RUNNER.read_text(encoding="utf-8")
base = BASE_RUNNER.read_text(encoding="utf-8")
fragment = FRAGMENT.read_text(encoding="utf-8")
makefile = MAKEFILE.read_text(encoding="utf-8")

for token in [
    "PHASE64_EXPECTED_HEAD",
    "work/phase64-reassignment-failover",
    "run_phase64_managed_timer_fulfillment_acceptance.sh",
    "PHASE_64_REASSIGNMENT_FAILOVER_ACCEPTANCE=PASS",
    "REASSIGNMENT=atomic-fail-closed",
    "OUTCOME_UNKNOWN=reconciliation-only",
    "PUBLIC_SVDRP_TIMER_WRITES=closed",
]:
    if token not in runner:
        raise SystemExit(f"reassignment acceptance runner missing: {token}")

for token in [
    "PHASE64_EXPECTED_BRANCH",
    "make -j2 test-architecture",
    "make -j2 test-ci-fast",
    'for command in ("NTCREATE", "NTMOD", "NTDELETE")',
    "ADVERTISEMENT=timer-commands-activated",
]:
    if token not in base:
        raise SystemExit(f"base acceptance contract missing: {token}")

for forbidden in ("git pull", "git fetch", "git push", "scp ", "curl ", "wget ", "systemctl ", "sqlite3 "):
    if forbidden in runner:
        raise SystemExit(f"reassignment acceptance crosses bounded scope: {forbidden}")

for token in [
    "phase64-reassignment-failover-acceptance",
    "run_phase64_reassignment_failover_acceptance.sh",
    "test-phase64-reassignment-failover-acceptance-architecture",
]:
    if token not in fragment:
        raise SystemExit(f"reassignment acceptance make fragment missing: {token}")

if "include mk/phase64-reassignment-failover-acceptance.mk" not in makefile:
    raise SystemExit("Makefile missing reassignment acceptance include")

print("Phase-64 reassignment/failover acceptance guard passed")
