#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "service_h": ROOT / "core/security/include/FirstAdminBootstrapIssuanceService.h",
    "service_cpp": ROOT / "core/security/src/FirstAdminBootstrapIssuanceService.cpp",
    "repository": ROOT / "core/security/src/FirstAdminBootstrapRepository.cpp",
    "tool": ROOT / "apps/tools/first_admin_bootstrap_issue.cpp",
    "test": ROOT / "core/security/tests/test_first_admin_bootstrap_issuance_service.cpp",
    "make": ROOT / "mk/security-sources.mk",
    "install": ROOT / "mk/install.mk",
    "man": ROOT / "docs/man/man8/vdr-suite-first-admin-bootstrap.8",
    "adr": ROOT / "docs/adr/ADR-0066-unclaimed-server-first-admin-bootstrap-recovery.md",
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
        "Only a local trusted operator may initiate first-admin bootstrap.",
        "root/operator-owned local command",
        "cryptographically random",
        "raw secret is shown once",
        "never persisted in plaintext",
    ):
        require("adr", marker)

    for marker in (
        "MinimumLifetimeSeconds = 300",
        "MaximumLifetimeSeconds = 3600",
        "DefaultLifetimeSeconds = 900",
        "IssuedFirstAdminBootstrap",
        "clearSecret",
        "EntropySource",
    ):
        require("service_h", marker)

    for marker in (
        "getrandom(",
        "SecretBytes = 32",
        '"$6$rounds=10000$"',
        '"fab_"',
        "repository_.registerBootstrap",
        "secureWipe",
    ):
        require("service_cpp", marker)

    for marker in (
        "geteuid() != 0",
        '"/var/lib/vdr-suite/vdr-suite.db"',
        '"bootstrap_id="',
        '"setup_secret="',
        '"expires_at="',
        "issued.clearSecret()",
        "FirstAdminBootstrapRepository",
        "FirstAdminBootstrapIssuanceService",
    ):
        require("tool", marker)

    for marker in (
        "test-security-first-admin-bootstrap-issuance-service:",
        "first-admin-bootstrap-issuer:",
        "python3 tools/check_p2_first_admin_bootstrap_issuer.py",
    ):
        require("make", marker)

    for marker in (
        "vdr-suite-first-admin-bootstrap",
        "first-admin-bootstrap-issuer",
    ):
        require("install", marker)

    require("man", "local root-only administration command")
    require("man", "setup_secret=...")
    require("man", "cannot be recovered from the database")

    for marker in (
        "stored.bootstrap.verifierHash",
        "issued.bootstrap->setupSecret",
        "FirstAdminBootstrapIssuanceStatus::conflict",
        "FirstAdminBootstrapIssuanceStatus::claimed",
        "FirstAdminBootstrapIssuanceStatus::entropyUnavailable",
    ):
        require("test", marker)

    for marker in (
        "local trusted-operator bootstrap issuer",
        "raw setup secret",
        "root-only",
        "no HTTP endpoint",
        "atomic first-admin claim service",
    ):
        require("audit", marker)

    forbid("service_cpp", "security_credentials")
    forbid("service_cpp", "security_sessions")
    forbid("tool", "Http")
    forbid("tool", "VDR_SUITE_SECURITY_MODE")
    forbid("tool", "ofstream")
    forbid("tool", "fopen(")
    forbid("repository", "setup_secret")

    print("P2 first-admin bootstrap issuer contracts passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"P2 first-admin bootstrap issuer check failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
