#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "runner": ROOT / "tools/p2_legacy_basic_retirement_acceptance.py",
    "roadmap": ROOT / "docs/planning/roadmap.md",
    "audit": ROOT / "docs/development/post-phase69-p2-first-admin-bootstrap-audit.md",
    "architecture": ROOT / "docs/architecture/security-identity-foundation.md",
    "runbook": ROOT / "docs/development/p2-legacy-basic-retirement-runtime-acceptance.md",
    "make": ROOT / "mk/security-sources.mk",
}

def read(name):
    path = FILES[name]
    if not path.exists():
        raise AssertionError(f"missing {path}")
    return path.read_text(encoding="utf-8")

def require(name, marker):
    if marker not in read(name):
        raise AssertionError(f"{FILES[name]} missing marker: {marker}")

def main():
    for marker in (
        "explicit_run_flag_required",
        "root_required",
        "unexpected_local_head",
        "unexpected_remote_ref",
        "worktree_not_clean",
        "human_password_preflight_failed",
        "existing_deployment_not_in_legacy_basic_mode",
        'atomic_write_security_mode(configuration, "enforced")',
        'atomic_write_security_mode(configuration, "legacy-basic")',
        "enforced_legacy_status",
        "rollback_legacy_status",
        "final_legacy_status",
        "persistent_identity_changed",
        "restore_configuration(",
        "restore_binary(",
        "FINAL_SECURITY_MODE=enforced",
        "P2_LEGACY_BASIC_RETIREMENT_RUNTIME_ACCEPTANCE=PASS",
    ):
        require("runner", marker)

    for marker in (
        "real yaVDR migration/rollback acceptance",
        "legacy-basic -> enforced -> legacy-basic -> enforced",
        "password is read interactively",
        "does not restart VDR",
        "failure restores",
    ):
        require("runbook", marker)

    for name, markers in {
        "roadmap": (
            "real deployment acceptance tooling",
            "real deployment execution remains pending",
        ),
        "audit": (
            "Legacy Basic retirement real-runtime acceptance tooling",
            "legacy-basic -> enforced -> legacy-basic -> enforced",
        ),
        "architecture": (
            "retirement runtime acceptance",
            "persistent Human Account identity fingerprint",
        ),
        "make": (
            "test-security-legacy-basic-retirement-acceptance",
            "check_p2_legacy_basic_retirement_acceptance.py",
        ),
    }.items():
        for marker in markers:
            require(name, marker)

    print("P2 Legacy Basic retirement runtime acceptance contracts passed")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"P2 Legacy Basic retirement acceptance check failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
