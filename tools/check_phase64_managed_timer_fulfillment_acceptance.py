#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
RUNNER = ROOT / "tools/run_phase64_managed_timer_fulfillment_acceptance.sh"
FRAGMENT = ROOT / "mk/phase64-managed-timer-fulfillment-acceptance.mk"
MAKEFILE = ROOT / "Makefile"

failures = []
for path in (RUNNER, FRAGMENT, MAKEFILE):
    if not path.is_file():
        failures.append(f"missing {path.relative_to(ROOT)}")

if RUNNER.is_file():
    text = RUNNER.read_text(encoding="utf-8")
    required = [
        "PHASE64_EXPECTED_HEAD",
        "exact_candidate_head_mismatch",
        "tracked_worktree_not_clean",
        "make -j2 test-architecture",
        "make -j2 test-ci-fast",
        "backend-agent-command-admin",
        "make -C vdr-plugin-suite-bridge -j2 check",
        "candidates.sha256",
        "COMMAND_TYPES=vdr.timer.create,vdr.timer.update,",
        "vdr.timer.toggle,vdr.timer.delete\\n",
        "if ! python3 -",
        (
            "PY_ACTIVATED_ADVERTISEMENT\n"
            "then\n"
            "    fail activated_advertisement_check_failed\n"
            "fi"
        ),
        "activated_advertisement_evidence_missing",
        'for command in ("NTCREATE", "NTMOD", "NTDELETE")',
        "ADVERTISEMENT=timer-commands-activated",
        "PHASE_64_MANAGED_TIMER_FULFILLMENT_ACCEPTANCE=PASS",
    ]
    for token in required:
        if token not in text:
            failures.append(f"acceptance runner missing contract: {token}")
    for forbidden in (
        "git pull",
        "git fetch",
        "git push",
        "scp ",
        "curl ",
        "wget ",
        "<<'PY_ACTIVATED_ADVERTISEMENT' ||",
        "systemctl ",
        "timers.conf",
        "sqlite3 ",
    ):
        if forbidden in text:
            failures.append(
                f"acceptance runner crosses bounded no-transfer/read-only scope: {forbidden}"
            )

if FRAGMENT.is_file():
    text = FRAGMENT.read_text(encoding="utf-8")
    for token in (
        "phase64-managed-timer-fulfillment-acceptance",
        "run_phase64_managed_timer_fulfillment_acceptance.sh",
    ):
        if token not in text:
            failures.append(f"acceptance make fragment missing: {token}")

if MAKEFILE.is_file() and (
    "include mk/phase64-managed-timer-fulfillment-acceptance.mk"
    not in MAKEFILE.read_text(encoding="utf-8")
):
    failures.append("Makefile missing Phase-64 acceptance include")

if failures:
    print("Phase-64 managed Timer acceptance guard failed:", file=sys.stderr)
    for failure in failures:
        print("- " + failure, file=sys.stderr)
    raise SystemExit(1)

print("Phase-64 managed Timer acceptance guard passed")
