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
        raise AssertionError(f"{path}: forbidden contract present: {needle}")


def main() -> int:
    header = "core/http/include/BrowserSessionHttpService.h"
    service = "core/http/src/BrowserSessionHttpService.cpp"
    server = "core/http/src/TestHttpServer.cpp"
    test = "core/http/tests/test_browser_session_http_service.cpp"
    document = (
        "docs/development/"
        "phase-62-slice-2s-browser-session-outcome-accountability.md"
    )

    for path in (header, service, server, test, document):
        if not (ROOT / path).is_file():
            raise AssertionError(f"missing Slice 2S contract file: {path}")

    require(header, "AccountabilityEventRepository& accountabilityRepository")
    require(header, "bool appendOutcome(")
    require(header, "mutable std::atomic<unsigned long long> idCounter_")

    require(service, '"operation.succeeded"')
    require(service, '"operation.failed"')
    require(service, '"session.issue.self"')
    require(service, '"browser.session.issue"')
    require(service, '"session.revoke.self"')
    require(service, '"browser.session.revoke"')
    require(service, '"browser_session_issued"')
    require(service, '"browser_session_revoked"')
    require(service, '"browser_session_issuance_failed"')
    require(service, '"browser_session_revocation_failed"')
    require(service, '"accountability_unavailable"')
    require(service, 'event.decision = "allowed"')
    require(service, 'event.outcome = succeeded ? "succeeded" : "failed"')
    require(service, 'event.backendId = "*"')
    require(service, "accountabilityRepository_.append(event)")
    require(
        service,
        "lifecycleService_.revoke(\n            issued->sessionId,\n            issued->credentialId)")
    require(service, "issued->clearSecrets()")
    require(service, "accountabilityUnavailableResponse(context, false)")
    require(service, "accountabilityUnavailableResponse(context, true)")
    require(service, 'response.headers["Set-Cookie"] = expiredSessionCookie()')
    forbid(service, "sessionCookieValue +")
    forbid(service, "csrfToken +")

    require(
        server,
        "*browserSessionLifecycleService_,\n            *accountabilityEventRepository_")

    require(test, "phase62_test_accountability_block")
    require(test, "request-outcome-blocked-login")
    require(test, "request-outcome-blocked-logout")
    require(test, "compensatedBrowser->revoked")
    require(test, "blockedLogoutBrowser->revoked")
    require(test, 'blockedLogout.headers.at("Set-Cookie").find("Max-Age=0")')
    require(test, '"operation.succeeded"')
    require(test, '"operation.failed"')
    require(test, "eventContains(event, cookieValue)")
    require(test, "eventContains(event, csrfToken)")

    require(document, "pre-dispatch accountability")
    require(document, "operation.succeeded")
    require(document, "operation.failed")
    require(document, "compensating revocation")
    require(document, "no `Set-Cookie` session credential")
    require(document, "expired `Set-Cookie`")
    require(document, "transactional outbox")
    require(document, "idle timeout")
    require(document, "expired-session cleanup")
    require(
        "docs/development/index.md",
        "phase-62-slice-2s-browser-session-outcome-accountability.md")

    print("browser session lifecycle outcome accountability contracts passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            "browser session outcome accountability architecture check failed: "
            f"{error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
