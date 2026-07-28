#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(path: str, needle: str) -> None:
    if needle not in read(path):
        raise AssertionError(f"{path}: missing required contract: {needle}")


def forbid(path: str, needle: str) -> None:
    if needle in read(path):
        raise AssertionError(f"{path}: forbidden contract remains active: {needle}")


def main() -> int:
    required_files = [
        "core/security/include/BrowserSessionIssuanceService.h",
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "core/security/src/SecurityIdentityIssuanceRepository.cpp",
        "core/security/tests/test_browser_session_issuance_service.cpp",
    ]
    for relative in required_files:
        if not (ROOT / relative).is_file():
            raise AssertionError(
                f"missing browser-session issuance contract file: {relative}")

    require(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "getrandom(")
    require(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        '"$6$rounds=10000$"')
    require(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        'database_.execute("BEGIN IMMEDIATE;")')
    require(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        'database_.execute("ROLLBACK;")')
    require(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        'database_.execute("COMMIT;")')
    require(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "identityRepository_.createSessionCredential(")
    require(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "credentialRepository_.insert(registration)")
    require(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "issuingCredential->expired")
    require(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "secureWipe(sessionCookieValue)")
    require(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "secureWipe(csrfToken)")
    require(
        "core/security/include/BrowserSessionIssuanceService.h",
        "IssuedBrowserSession(const IssuedBrowserSession&) = delete")
    require(
        "core/security/include/BrowserSessionIssuanceService.h",
        "MaximumLifetimeSeconds = 86400")
    require(
        "core/security/src/SecurityIdentityIssuanceRepository.cpp",
        "INSERT INTO security_sessions")
    require(
        "core/security/src/SecurityIdentityIssuanceRepository.cpp",
        "INSERT INTO security_credentials")
    require(
        "core/security/src/SecurityIdentityIssuanceRepository.cpp",
        "rotated_from_credential_id")
    require(
        "core/security/tests/test_browser_session_issuance_service.cpp",
        "rolledBackSessionId")
    require(
        "core/security/tests/test_browser_session_issuance_service.cpp",
        "revokedSourceService")
    require(
        "core/security/tests/test_browser_session_issuance_service.cpp",
        "authenticator.verifyCsrf(headers)")
    require(
        "mk/security-sources.mk",
        "test-security-browser-session-issuance-service")
    require(
        "mk/security-sources.mk",
        "core/security/src/BrowserSessionIssuanceService.cpp")
    require(
        "mk/daemon-sources.mk",
        "DAEMON_SRC += $(SECURITY_SRC)")

    forbid(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "sqlite3_")
    forbid(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "std::cout")
    forbid(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "std::cerr")
    forbid(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "RuntimeLogger")
    forbid(
        "core/http/src/TestHttpServer.cpp",
        "BrowserSessionIssuanceService")
    forbid(
        "core/http/src/TestHttpServer.cpp",
        "BrowserSessionAuthenticator")
    forbid(
        "core/http/src/TestHttpServer.cpp",
        "vdr_suite_session=")
    forbid(
        "core/security/include/SecurityHttpGate.h",
        "BrowserSessionIssuanceService")
    forbid(
        "core/security/include/SecurityHttpGate.h",
        "BrowserSessionAuthenticator")
    forbid(
        "core/security/src/BrowserSessionCredentialRepository.cpp",
        "session_secret TEXT")
    forbid(
        "core/security/src/BrowserSessionCredentialRepository.cpp",
        "csrf_secret TEXT")
    forbid(
        "core/security/src/BrowserSessionCredentialRepository.cpp",
        "cookie_value")

    print("browser session issuance architecture contracts passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"browser session issuance architecture check failed: {error}",
            file=sys.stderr)
        raise SystemExit(1)
