#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "header": ROOT / "core/security/include/FirstAdminBootstrapRepository.h",
    "source": ROOT / "core/security/src/FirstAdminBootstrapRepository.cpp",
    "test": ROOT / "core/security/tests/test_first_admin_bootstrap_repository.cpp",
    "daemon_h": ROOT / "core/daemon/include/DaemonRuntime.h",
    "daemon_init": ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp",
    "make": ROOT / "mk/security-sources.mk",
    "audit": ROOT / "docs/development/post-phase69-p2-first-admin-bootstrap-audit.md",
}


def read(name):
    path = FILES[name]
    if not path.exists():
        raise AssertionError(f"missing {path}")
    return path.read_text(encoding="utf-8")


def require(name, marker):
    if marker not in read(name):
        raise AssertionError(f"{FILES[name]} missing marker: {marker}")


def forbid(name, marker):
    if marker in read(name):
        raise AssertionError(f"{FILES[name]} contains forbidden marker: {marker}")


def main():
    for marker in (
        "FirstAdminClaimState",
        "FirstAdminBootstrapStatus",
        "registerBootstrap",
        "consumeInActiveTransaction",
        "invalidateInActiveTransaction",
        "supportsVerifierHash",
    ):
        require("header", marker)

    for marker in (
        "security_first_admin_bootstrap_issuances",
        "verifier_hash",
        "expires_at",
        "consumed_at",
        "invalidated_at",
        'database_.execute("BEGIN IMMEDIATE;")',
        "database_.transactionActive()",
        "security_human_accounts",
        "security_actor_permission_grants",
        "grant_record.backend_id = '*'",
        "grant_record.permission = 'role.admin'",
        "grant_record.permission = '*'",
        'verifierHash.rfind("$y$", 0)',
        'verifierHash.rfind("$6$", 0)',
        "expires_at > CURRENT_TIMESTAMP",
        "expires_at <= CURRENT_TIMESTAMP",
    ):
        require("source", marker)

    for marker in (
        "plain-text-bootstrap-secret",
        "transactionRequired",
        'database.execute("ROLLBACK;")',
        'database.execute("COMMIT;")',
        "FirstAdminClaimState::claimed",
        "FirstAdminBootstrapStatus::claimed",
        "FirstAdminBootstrapStatus::consumed",
        "FirstAdminBootstrapStatus::invalidated",
    ):
        require("test", marker)

    require("daemon_h", '#include "FirstAdminBootstrapRepository.h"')
    require("daemon_h", "firstAdminBootstrapRepository_")
    require(
        "daemon_init",
        "std::make_unique<FirstAdminBootstrapRepository>(database_)",
    )
    require("daemon_init", "firstAdminBootstrapRepository_->ensureSchema()")

    require(
        "make",
        "core/security/src/FirstAdminBootstrapRepository.cpp",
    )
    require("make", "test-security-first-admin-bootstrap-repository:")
    require(
        "make",
        "python3 tools/check_p2_first_admin_bootstrap_runtime.py",
    )

    for marker in (
        "FirstAdminBootstrapRepository",
        "claim state remains derived",
        "no normal Session",
        "Credential row",
        "caller-owned SQLite transaction",
        "Legacy Basic default remains unchanged",
    ):
        require("audit", marker)

    forbid("source", "INSERT INTO security_credentials")
    forbid("source", "INSERT INTO security_sessions")
    forbid("source", "plaintext")
    forbid("daemon_init", "VDR_SUITE_SECURITY_MODE")

    print("P2 first-admin bootstrap runtime foundation contracts passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"P2 first-admin bootstrap runtime foundation check failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
