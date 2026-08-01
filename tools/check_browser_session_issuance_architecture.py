#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(path: str, needle: str) -> None:
    if needle not in read(path):
        raise AssertionError(f"{path}: missing required contract: {needle}")


def require_any(path: str, alternatives: tuple[str, ...]) -> None:
    text = read(path)
    if not any(alternative in text for alternative in alternatives):
        joined = " or ".join(repr(value) for value in alternatives)
        raise AssertionError(
            f"{path}: missing required alternative contract: {joined}")


def forbid(path: str, needle: str) -> None:
    if needle in read(path):
        raise AssertionError(f"{path}: forbidden contract remains active: {needle}")


def require_count(path: str, needle: str, count: int) -> None:
    actual = read(path).count(needle)
    if actual != count:
        raise AssertionError(
            f"{path}: expected {count} occurrences of {needle}, found {actual}")


def main() -> int:
    lifetime_document = (
        "docs/development/"
        "phase-62-slice-2r-browser-session-lifetime-configuration.md"
    )
    required_files = [
        "core/security/include/BrowserSessionIssuanceService.h",
        "core/security/include/BrowserSessionLifecycleService.h",
        "core/security/include/BrowserSessionHttpGate.h",
        "core/security/include/SecurityConfiguration.h",
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "core/security/src/BrowserSessionLifecycleService.cpp",
        "core/security/src/BrowserSessionHttpGate.cpp",
        "core/security/src/SecurityIdentityIssuanceRepository.cpp",
        "core/http/include/BrowserSessionHttpService.h",
        "core/http/src/BrowserSessionHttpService.cpp",
        "core/security/tests/test_browser_session_issuance_service.cpp",
        "core/security/tests/test_browser_session_http_gate.cpp",
        "core/security/tests/test_security_configuration.cpp",
        "core/http/tests/test_browser_session_http_service.cpp",
        "packaging/systemd/vdr-suite-daemon.default",
        lifetime_document,
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
        "MinimumLifetimeSeconds = 300")
    require(
        "core/security/include/BrowserSessionIssuanceService.h",
        "DefaultLifetimeSeconds = 28800")
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

    configuration = "core/security/include/SecurityConfiguration.h"
    configuration_test = "core/security/tests/test_security_configuration.cpp"
    service = "core/http/src/BrowserSessionHttpService.cpp"
    service_test = "core/http/tests/test_browser_session_http_service.cpp"
    runtime_default = "packaging/systemd/vdr-suite-daemon.default"
    lifetime_variable = "VDR_SUITE_BROWSER_SESSION_LIFETIME_SECONDS"

    require(configuration, "BrowserSessionLifetimeConfiguration")
    require(configuration, "configuredValueValid")
    require(configuration, "parseBrowserSessionLifetime")
    require_count(configuration, lifetime_variable, 1)
    require(
        configuration,
        "BrowserSessionIssuanceService::MinimumLifetimeSeconds")
    require(
        configuration,
        "BrowserSessionIssuanceService::DefaultLifetimeSeconds")
    require(
        configuration,
        "BrowserSessionIssuanceService::MaximumLifetimeSeconds")
    require(configuration, "parsed > (maximum - digit) / 10")

    require(configuration_test, '"900"')
    require(configuration_test, '"300"')
    require(configuration_test, '"86400"')
    require(configuration_test, '"299"')
    require(configuration_test, '"86401"')
    require(configuration_test, '"+3600"')
    require(configuration_test, '" 3600"')
    require(configuration_test, '"3600x"')
    require(
        configuration_test,
        '"999999999999999999999999999999999999"')

    require_any(
        service,
        (
            "SecurityConfiguration::fromEnvironment().browserSessionLifetime",
            "const SecurityConfiguration configuration =\n"
            "        SecurityConfiguration::fromEnvironment();",
        ))
    if "SecurityConfiguration::fromEnvironment().browserSessionLifetime" not in read(service):
        require(service, "lifetimeConfiguration_ = configuration.browserSessionLifetime")
        require(
            service,
            "concurrencyConfiguration_ = configuration.browserSessionConcurrency")
    require(service, "lifetimeConfiguration_.valid()")
    require(
        service,
        "request.lifetimeSeconds = lifetimeConfiguration_.seconds")
    require(service, "lifetimeConfiguration_.seconds);")
    require(
        service,
        '"browser_session_lifetime_configuration_invalid"')
    require(
        service,
        'response.headers["Set-Cookie"]')
    require(service, '"; Path=/; Max-Age="')
    require(service, '"; HttpOnly; Secure; SameSite=Strict"')
    require(service, 'response.headers["Cache-Control"] = "no-store"')
    require(service, 'response.headers["Pragma"] = "no-cache"')
    require(service, "csrfToken")
    require(service, "issued->clearSecrets()")
    forbid(service, lifetime_variable)
    forbid(service, "DefaultLifetimeSeconds")
    forbid(service, "Domain=")

    require(service_test, '"; Max-Age=900"')
    require(service_test, '"2099-01-01 00:15:00"')
    require(
        service_test,
        '"browser_session_lifetime_configuration_invalid"')
    require(service_test, "configuredValueValid = false")
    require(service_test, "belowMinimumConfiguration.seconds = 299")

    require_count(runtime_default, lifetime_variable, 1)
    require(runtime_default, f"{lifetime_variable}=28800")
    require(runtime_default, "strict decimal")
    require(runtime_default, "300 through 86400")
    require(runtime_default, "not an idle timeout")

    require(
        lifetime_document,
        f"`{lifetime_variable}`")
    require(lifetime_document, "default: 28800 seconds")
    require(lifetime_document, "minimum:   300 seconds")
    require(lifetime_document, "maximum: 86400 seconds")
    require(lifetime_document, "no `Set-Cookie` header")
    require(lifetime_document, "idle timeout")
    require(
        "docs/development/index.md",
        "phase-62-slice-2r-browser-session-lifetime-configuration.md")

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
    require(
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
    require(service_test, "SameSite=Strict")
    require(
        service_test,
        "login.body.find(cookieValue) == std::string::npos")
    require(service_test, "revokedBrowser->active")

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
    require(
        "core/security/include/SecurityHttpGate.h",
        "BrowserSessionAuthenticator")
    require(
        "core/security/include/SecurityHttpGate.h",
        "browserSessionAuthenticator_->hasSessionCookie(")
    require(
        "core/security/include/SecurityHttpGate.h",
        "browserSessionAuthenticator_->authenticate(")
    require(
        "core/security/include/SecurityHttpGate.h",
        '"http.browser.mutation"')
    require(
        "core/security/include/SecurityHttpGate.h",
        "browserSessionAuthenticator_->verifyCsrf(")
    require(
        "core/security/include/SecurityHttpGate.h",
        '"csrf_validation_failed"')
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
