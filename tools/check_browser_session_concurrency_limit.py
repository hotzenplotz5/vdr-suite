#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "configuration": ROOT / "core/security/include/SecurityConfiguration.h",
    "repository_header": ROOT / "core/security/include/BrowserSessionCredentialRepository.h",
    "repository_source": ROOT / "core/security/src/BrowserSessionCredentialRepository.cpp",
    "issuance_header": ROOT / "core/security/include/BrowserSessionIssuanceService.h",
    "issuance_source": ROOT / "core/security/src/BrowserSessionIssuanceService.cpp",
    "http_header": ROOT / "core/http/include/BrowserSessionHttpService.h",
    "http_source": ROOT / "core/http/src/BrowserSessionHttpService.cpp",
    "packaging": ROOT / "packaging/systemd/vdr-suite-daemon.default",
    "contract": ROOT / "docs/development/phase-62-slice-2u-browser-session-concurrency-limit.md",
    "test": ROOT / "core/security/tests/test_browser_session_concurrency_limit.cpp",
    "make": ROOT / "mk/security-sources.mk",
}

REQUIRED = {
    "configuration": [
        "BrowserSessionConcurrencyConfiguration",
        "maximumActivePerActor = 0",
        "VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR",
        "parseBrowserSessionMaximum",
        "MaximumActiveSessionsPerActor",
    ],
    "repository_header": [
        "countEffectiveActiveByActorId",
        "std::optional<std::size_t>",
    ],
    "repository_source": [
        "SELECT COUNT(*)",
        "security_browser_session_credentials AS browser",
        "security_actors AS actor",
        "security_devices AS device",
        "security_sessions AS session",
        "security_credentials AS browser_credential",
        "security_credentials AS issuing_credential",
        "browser.expires_at > CURRENT_TIMESTAMP",
        "session.expires_at > CURRENT_TIMESTAMP",
        "browser_credential.expires_at > CURRENT_TIMESTAMP",
        "issuing_credential.expires_at > CURRENT_TIMESTAMP",
    ],
    "issuance_header": [
        "maximumActivePerActor = 0",
        "MaximumActiveSessionsPerActor = 64",
        "BrowserSessionIssuanceStatus",
        "LimitReached",
        "issueWithPolicy",
    ],
    "issuance_source": [
        "BEGIN IMMEDIATE;",
        "countEffectiveActiveByActorId",
        "BrowserSessionIssuanceStatus::LimitReached",
        "createSessionCredential",
        "credentialRepository_.insert",
    ],
    "http_header": [
        "BrowserSessionConcurrencyConfiguration",
        "concurrencyConfiguration_",
    ],
    "http_source": [
        "browser_session_limit_configuration_invalid",
        "browser_session_limit_reached",
        "BrowserSessionIssuanceStatus::LimitReached",
        "errorResponse(\n            409,",
        "request.maximumActivePerActor",
    ],
    "packaging": [
        "VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR=0",
        "never evicts an",
    ],
    "contract": [
        "Concurrent Browser-Session Limit",
        "count < N",
        "count >= N",
        "HTTP 409",
        "automatic eviction",
        "zero VDR",
    ],
    "test": [
        "BrowserSessionIssuanceStatus::LimitReached",
        "countEffectiveActiveByActorId",
        "browser_session_limit_reached",
        "browser_session_limit_configuration_invalid",
        "rawAfterIssuerRevoke->active",
    ],
    "make": [
        "check_browser_session_concurrency_limit.py",
        "test-security-browser-session-concurrency-limit",
        "test_browser_session_concurrency_limit.cpp",
    ],
}


def fail(message: str) -> None:
    print("Browser-session concurrency architecture check failed:")
    print("- " + message)
    raise SystemExit(1)


def main() -> int:
    texts = {}
    for name, path in FILES.items():
        if not path.exists():
            fail(f"missing file: {path.relative_to(ROOT)}")
        texts[name] = path.read_text(encoding="utf-8")

    for name, markers in REQUIRED.items():
        for marker in markers:
            if marker not in texts[name]:
                fail(f"{FILES[name].relative_to(ROOT)} missing {marker!r}")

    issuance = texts["issuance_source"]
    transaction_at = issuance.find("BEGIN IMMEDIATE;")
    count_at = issuance.find("countEffectiveActiveByActorId")
    insert_at = issuance.find("createSessionCredential")
    commit_at = issuance.find("transaction.commit()")

    if not (
        transaction_at >= 0
        and count_at > transaction_at
        and insert_at > count_at
        and commit_at > insert_at
    ):
        fail("effective count must stay inside the issuance transaction before inserts")

    count_method = texts["repository_source"]
    count_start = count_method.find("countEffectiveActiveByActorId")
    count_end = count_method.find("touchLastSeenIfDue", count_start)
    if count_end < 0:
        count_end = count_method.find("revokeBySessionId", count_start)
    if count_start < 0 or count_end < 0:
        fail("could not isolate effective-count implementation")
    count_body = count_method[count_start:count_end]

    for forbidden in ("UPDATE ", "DELETE ", "revoked_at = CURRENT_TIMESTAMP"):
        if forbidden in count_body:
            fail(f"effective count must be read-only; found {forbidden!r}")

    http = texts["http_source"]
    limit_at = http.find("BrowserSessionIssuanceStatus::LimitReached")
    status_at = http.find("errorResponse(\n            409,", limit_at)
    if limit_at < 0 or status_at < limit_at:
        fail("only the explicit LimitReached result may map to HTTP 409")

    packaging = texts["packaging"]
    if "VDR_SUITE_BROWSER_SESSION_MAX_ACTIVE_PER_ACTOR=1" in packaging:
        fail("compatibility default must remain unlimited")

    print("Browser-session concurrency architecture check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
