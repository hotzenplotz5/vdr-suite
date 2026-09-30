#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "service_h": ROOT / "core/security/include/HumanAccountRecoveryService.h",
    "service_cpp": ROOT / "core/security/src/HumanAccountRecoveryService.cpp",
    "browser_h": ROOT / "core/security/include/BrowserSessionCredentialRepository.h",
    "browser_cpp": ROOT / "core/security/src/BrowserSessionCredentialRepository.cpp",
    "tool": ROOT / "apps/tools/human_account_recover.cpp",
    "test": ROOT / "core/security/tests/test_human_account_recovery_service.cpp",
    "make": ROOT / "mk/security-sources.mk",
    "install": ROOT / "mk/install.mk",
    "man": ROOT / "docs/man/man8/vdr-suite-human-account-recover.8",
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
        "HumanAccountRecoveryRequest",
        "HumanAccountRecoveryResult",
        "HumanAccountRecoveryStatus",
        "revokedBrowserSessions",
        "EntropySource",
        "clearSecrets",
    ):
        require("service_h", marker)

    for marker in (
        'database_.execute("BEGIN IMMEDIATE;")',
        "humanAccountRepository_.findByAccountId",
        "credentialVerifierRepository_.findByLogin",
        'credential->credentialType != "human-password"',
        "crypt_gensalt_rn(",
        '"$y$"',
        "credentialVerifierRepository_.updateVerifier",
        "findByIssuedFromCredentialId",
        "revokeBySessionId",
        "identityRepository_.revokeSession",
        "identityRepository_.revokeCredential",
        '"security.human-account.recovery"',
        '"local-root"',
        '"local_root_password_reset"',
        "accountabilityRepository_.append(event)",
        "transaction.commit()",
    ):
        require("service_cpp", marker)

    for marker in (
        "findByIssuedFromCredentialId",
        "std::vector<StoredBrowserSessionCredential>",
    ):
        require("browser_h", marker)

    for marker in (
        "idx_security_browser_sessions_issuer",
        "issued_from_credential_id = ?",
        "AND active <> 0 AND revoked_at = ''",
    ):
        require("browser_cpp", marker)

    for marker in (
        "geteuid() != 0",
        '"--account-id"',
        '"--login"',
        '"/var/lib/vdr-suite/vdr-suite.db"',
        "isatty(STDIN_FILENO)",
        "tcsetattr(",
        "HumanAccountRecoveryService",
        '"revoked_browser_sessions="',
    ):
        require("tool", marker)

    for marker in (
        "HumanAccountRecoveryStatus::success",
        'after->passwordHash.rfind("$y$", 0) == 0',
        "!passwordMatches(OldPassword",
        "passwordMatches(NewPassword",
        "result.revokedBrowserSessions == 2",
        '"security.human-account.recovery"',
        '"account_not_found"',
        '"account_inactive"',
        '"credential_invalid"',
        '"test.duplicate"',
        'events[0].eventType == "test.duplicate"',
    ):
        require("test", marker)

    for marker in (
        "human-account-recovery:",
        "test-security-human-account-recovery:",
        "python3 tools/check_p2_human_account_recovery.py",
        "core/security/src/HumanAccountRecoveryService.cpp",
    ):
        require("make", marker)

    for marker in (
        "human-account-recovery",
        "vdr-suite-human-account-recover",
        "vdr-suite-human-account-recover.8",
    ):
        require("install", marker)

    for marker in (
        "local root-only administration command",
        "accept the new password as a command-line argument",
        "issued from that human-password credential are revoked",
        "does not create a second Human Account",
    ):
        require("man", marker)

    for marker in (
        "Local audited Human Account recovery",
        "same human-password credential",
        "existing browser sessions",
        "Authentication-default migration remains separate",
    ):
        require("adr", marker)

    for marker in (
        "local audited Human Account recovery",
        "direct local credential reset",
        "issued_from_credential_id",
        "no remote recovery endpoint",
    ):
        require("audit", marker)

    for name in ("service_h", "service_cpp", "tool"):
        forbid(name, "/api/v1")
        forbid(name, "FirstAdminBootstrap")
    forbid("service_cpp", "INSERT INTO security_")
    forbid("service_cpp", "UPDATE security_")
    forbid("service_cpp", "DELETE FROM security_")
    forbid("tool", '"--password')
    forbid("tool", "Http")

    print("P2 local Human Account recovery contracts passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"P2 local Human Account recovery check failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
