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


def require_count(path: str, needle: str, count: int) -> None:
    actual = read(path).count(needle)
    if actual != count:
        raise AssertionError(
            f"{path}: expected {count} occurrences of {needle}, found {actual}")


def main() -> int:
    required_files = [
        "core/security/include/BrowserSessionIssuanceService.h",
        "core/security/include/BrowserSessionLifecycleService.h",
        "core/security/include/BrowserSessionHttpGate.h",
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "core/security/src/BrowserSessionLifecycleService.cpp",
        "core/security/src/BrowserSessionHttpGate.cpp",
        "core/security/src/SecurityIdentityIssuanceRepository.cpp",
        "core/http/include/BrowserSessionHttpService.h",
        "core/http/src/BrowserSessionHttpService.cpp",
        "core/security/tests/test_browser_session_issuance_service.cpp",
        "core/security/tests/test_browser_session_http_gate.cpp",
        "core/http/tests/test_browser_session_http_service.cpp",
    ]
    for relative in required_files:
        if not (ROOT / relative).is_file():
            raise AssertionError(
                f"missing browser-session contract file: {relative}")

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
        "core/security/src/BrowserSessionLifecycleService.cpp",
        'database_.execute("BEGIN IMMEDIATE;")')
    require(
        "core/security/src/BrowserSessionLifecycleService.cpp",
        'database_.execute("ROLLBACK;")')
    require(
        "core/security/src/BrowserSessionLifecycleService.cpp",
        'database_.execute("COMMIT;")')
    require(
        "core/security/src/BrowserSessionLifecycleService.cpp",
        "credentialRepository_.revokeBySessionId(sessionId)")
    require(
        "core/security/src/BrowserSessionLifecycleService.cpp",
        "identityRepository_.revokeSession(sessionId)")
    require(
        "core/security/src/BrowserSessionLifecycleService.cpp",
        "identityRepository_.revokeCredential(credentialId)")

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
        "core/http/src/BrowserSessionHttpService.cpp",
        'response.headers["Set-Cookie"]')
    require(
        "core/http/src/BrowserSessionHttpService.cpp",
        '"; Path=/; Max-Age="')
    require(
        "core/http/src/BrowserSessionHttpService.cpp",
        '"; HttpOnly; Secure; SameSite=Strict"')
    require(
        "core/http/src/BrowserSessionHttpService.cpp",
        'response.headers["Cache-Control"] = "no-store"')
    require(
        "core/http/src/BrowserSessionHttpService.cpp",
        'response.headers["Pragma"] = "no-cache"')
    require(
        "core/http/src/BrowserSessionHttpService.cpp",
        '"csrfToken"')
    require(
        "core/http/src/BrowserSessionHttpService.cpp",
        "issued->clearSecrets()")
    forbid(
        "core/http/src/BrowserSessionHttpService.cpp",
        "Domain=")

    require(
        "core/security/src/BrowserSessionHttpGate.cpp",
        '"/api/security/browser-sessions"')
    require(
        "core/security/src/BrowserSessionHttpGate.cpp",
        '"/api/security/browser-sessions/logout"')
    require(
        "core/security/src/BrowserSessionHttpGate.cpp",
        'request.method != "POST"')
    require(
        "core/security/src/BrowserSessionHttpGate.cpp",
        "authenticateBasic(request)")
    require(
        "core/security/src/BrowserSessionHttpGate.cpp",
        "authenticateBrowser(request)")
    require_count(
        "core/security/src/BrowserSessionHttpGate.cpp",
        "browserAuthenticator_->authenticate(",
        1)
    require_count(
        "core/security/src/BrowserSessionHttpGate.cpp",
        "browserAuthenticator_->verifyCsrf(",
        1)
    require(
        "core/security/src/BrowserSessionHttpGate.cpp",
        '"session.issue.self"')
    require(
        "core/security/src/BrowserSessionHttpGate.cpp",
        '"session.revoke.self"')
    require(
        "core/security/src/BrowserSessionHttpGate.cpp",
        '"csrf_validation_failed"')

    server = read("core/http/src/TestHttpServer.cpp")
    browser_gate_position = server.find(
        "browserSessionHttpGate_->handles(request)")
    normal_gate_position = server.find(
        "securityHttpGate_->evaluate(request)")
    if min(browser_gate_position, normal_gate_position) < 0 or not (
            browser_gate_position < normal_gate_position):
        raise AssertionError(
            "TestHttpServer: browser lifecycle gate must precede the normal gate")
    require(
        "core/http/src/TestHttpServer.cpp",
        "browserSessionHttpService_->login(browserGate.context)")
    require(
        "core/http/src/TestHttpServer.cpp",
        "browserSessionHttpService_->logout(browserGate.context)")
    forbid(
        "core/http/src/TestHttpServer.cpp",
        "BrowserSessionAuthenticator")
    forbid(
        "core/http/src/TestHttpServer.cpp",
        "vdr_suite_session=")

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
        "core/security/tests/test_browser_session_http_gate.cpp",
        "ordinaryGet")
    require(
        "core/security/tests/test_browser_session_http_gate.cpp",
        "remotePost")
    require(
        "core/security/tests/test_browser_session_http_gate.cpp",
        "csrf_validation_failed")
    require(
        "core/http/tests/test_browser_session_http_service.cpp",
        "SameSite=Strict")
    require(
        "core/http/tests/test_browser_session_http_service.cpp",
        "login.body.find(cookieValue) == std::string::npos")
    require(
        "core/http/tests/test_browser_session_http_service.cpp",
        "revokedBrowser->active")

    require(
        "mk/security-sources.mk",
        "test-security-browser-session-issuance-service")
    require(
        "mk/security-sources.mk",
        "test-security-browser-session-http-service")
    require(
        "mk/security-sources.mk",
        "test-security-browser-session-http-gate")
    require(
        "mk/security-sources.mk",
        "core/security/src/BrowserSessionLifecycleService.cpp")
    require(
        "mk/security-sources.mk",
        "core/security/src/BrowserSessionHttpGate.cpp")
    require(
        "mk/daemon-sources.mk",
        "core/http/src/BrowserSessionHttpService.cpp")
    require(
        "mk/daemon-sources.mk",
        "DAEMON_SRC += $(SECURITY_SRC)")

    forbid(
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "sqlite3_")
    forbid(
        "core/security/src/BrowserSessionLifecycleService.cpp",
        "sqlite3_")
    forbid(
        "core/security/include/SecurityHttpGate.h",
        "BrowserSessionIssuanceService")
    forbid(
        "core/security/include/SecurityHttpGate.h",
        "BrowserSessionAuthenticator")
    forbid(
        "core/security/include/SecurityHttpGate.h",
        "vdr_suite_session")
    forbid(
        "core/security/src/BrowserSessionCredentialRepository.cpp",
        "session_secret TEXT")
    forbid(
        "core/security/src/BrowserSessionCredentialRepository.cpp",
        "csrf_secret TEXT")
    forbid(
        "core/security/src/BrowserSessionCredentialRepository.cpp",
        "cookie_value")

    print("browser session issuance and HTTP lifecycle contracts passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"browser session issuance architecture check failed: {error}",
            file=sys.stderr)
        raise SystemExit(1)
