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
        "require_package_maintenance_idle(root)",
        "PACKAGE_MAINTENANCE_SERVICES",
        "apt-daily-upgrade.service",
        "PACKAGE_MANAGER_LOCKS",
        "package_maintenance_active:",
        "package_manager_lock_held:",
        "CANDIDATE_REUSE_ONLY_PATHS",
        "validate_candidate_source_head(",
        "candidate_source_head_not_ancestor",
        "candidate_reuse_touches_daemon_inputs:",
        "--candidate-source-head",
        "CANDIDATE_SOURCE_HEAD=",
        "security_integrity_tables(",
        "sqlite_security_integrity_scope_empty",
        "PRAGMA quick_check({quoted_table})",
        "PRAGMA foreign_key_check({quoted_table})",
        "initial_sqlite_integrity_tables",
        "final_sqlite_integrity_tables",
        "command_failed:{command}:exit_",
        "human_password_preflight_failed",
        "system_libcrypt_unavailable",
        "LEGACY_AUTH_PROBE_PATH",
        "/api/v1/operations/p2-retirement-legacy-auth-probe-never-created",
        "legacy_auth_rejected_status_",
        "legacy_auth_accepted_status_",
        "Create exactly one persistent VDR-Suite First Admin Human Account.",
        "fields of that same account.",
        "existing_deployment_not_in_legacy_basic_mode",
        "first_admin_required:",
        "--bootstrap-first-admin",
        "candidate_bootstrap_tool_fingerprint_changed",
        "server_claimed(",
        "candidate_human_account_schema_missing",
        "first_admin_created",
        'atomic_write_security_mode(configuration, "enforced")',
        'atomic_write_security_mode(configuration, "legacy-basic")',
        "enforced_legacy_status",
        "rollback_legacy_status",
        "final_legacy_status",
        "persistent_identity_changed",
        "candidate_evidence_fingerprint_changed",
        "vdr-suite-daemon.candidate",
        "restore_configuration(",
        "restore_binary(",
        "FINAL_SECURITY_MODE=enforced",
        "P2_LEGACY_BASIC_RETIREMENT_RUNTIME_ACCEPTANCE=PASS",
    ):
        require("runner", marker)

    for marker in (
        "real yaVDR migration/rollback acceptance",
        "legacy-basic -> enforced -> legacy-basic -> enforced",
        "protected Public-v1 Operation read",
        "while HTTP 401 proves that the old",
        "compatibility credential no longer establishes an identity in enforced mode.",
        "password is read interactively",
        "pre-P2",
        "--bootstrap-first-admin",
        "creates exactly one persistent VDR-Suite First Admin Human Account",
        "They are not separate accounts",
        "successful First Admin claim persists",
        "package-maintenance preflight",
        "apt-daily-upgrade.service",
        "the runner rechecks immediately before",
        "the first service mutation. A package-maintenance collision therefore fails",
        "candidate source head",
        "only acceptance-runner, guard and runbook files",
        "security-scoped SQLite integrity",
        "scan the complete production database, because Recording, EPG and media-cache",
        "does not restart VDR",
        "failure restores",
        "## Accepted real yaVDR execution",
        "acceptance_head=716dbbdceb95aa9c6ea93e169df2ac7364a65be7",
        "P2_LEGACY_BASIC_RETIREMENT_RUNTIME_ACCEPTANCE=PASS",
        "PERSISTENT_IDENTITY_UNCHANGED=PASS",
        "FINAL_SECURITY_MODE=enforced",
    ):
        require("runbook", marker)

    for name, markers in {
        "roadmap": (
            "Real-deployment migration gate accepted",
            "supported real yaVDR deployment completed",
            "historical migration evidence",
        ),
        "audit": (
            "Legacy Basic retirement real-runtime acceptance tooling",
            "legacy-basic -> enforced -> legacy-basic -> enforced",
            "completed the guarded sequence successfully",
            "This satisfies the real deployment migration/rollback gate.",
            "Legacy Basic runtime implementation removal",
        ),
        "architecture": (
            "guarded real yaVDR migration/rollback acceptance completed successfully",
            "Historical retirement\nacceptance evidence remains retained",
            "Legacy Basic runtime compatibility is now removed",
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
