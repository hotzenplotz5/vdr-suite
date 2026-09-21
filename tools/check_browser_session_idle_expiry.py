#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "configuration": ROOT / "core/security/include/SecurityConfiguration.h",
    "repository_header": ROOT / "core/security/include/BrowserSessionCredentialRepository.h",
    "repository_source": ROOT / "core/security/src/BrowserSessionCredentialRepository.cpp",
    "authenticator": ROOT / "core/security/include/BrowserSessionAuthenticator.h",
    "issuance_header": ROOT / "core/security/include/BrowserSessionIssuanceService.h",
    "issuance_source": ROOT / "core/security/src/BrowserSessionIssuanceService.cpp",
    "http_header": ROOT / "core/http/include/BrowserSessionHttpService.h",
    "http_source": ROOT / "core/http/src/BrowserSessionHttpService.cpp",
    "lifecycle_gate": ROOT / "core/security/src/BrowserSessionHttpGate.cpp",
    "server": ROOT / "core/http/src/TestHttpServer.cpp",
    "packaging": ROOT / "packaging/systemd/vdr-suite-daemon.default",
    "contract": ROOT / "docs/development/phase-62-slice-2v-browser-session-idle-expiry.md",
    "test": ROOT / "core/security/tests/test_browser_session_idle_expiry.cpp",
    "make": ROOT / "mk/security-sources.mk",
}

REQUIRED = {
    "configuration": [
        "BrowserSessionIdleConfiguration",
        "timeoutSeconds = 0",
        "MinimumTimeoutSeconds = 300",
        "MaximumTimeoutSeconds = 86400",
        "LastSeenWriteIntervalSeconds = 60",
        "VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS",
        "parseBrowserSessionIdleTimeout",
    ],
    "repository_header": [
        "lastSeenAt",
        "idleExpired",
        "findResolvedByTokenId",
        "touchLastSeenIfDue",
        "countEffectiveActiveByActorId",
    ],
    "repository_source": [
        "ADD COLUMN last_seen_at",
        "SET last_seen_at = created_at",
        "last_seen_at TEXT NOT NULL",
        "browser.last_seen_at <=",
        "browser.last_seen_at >",
        "touchLastSeenIfDue",
        "SET last_seen_at = CURRENT_TIMESTAMP",
        "datetime(CURRENT_TIMESTAMP, '-' || ?2 || ' seconds')",
    ],
    "authenticator": [
        "idleTimeoutSeconds_",
        "lastSeenWriteIntervalSeconds_",
        "record->idleExpired",
        "touchLastSeenIfDue",
        "verifyCsrf",
    ],
    "issuance_header": [
        "idleTimeoutSeconds = 0",
        "MinimumIdleTimeoutSeconds = 300",
        "MaximumIdleTimeoutSeconds = 86400",
    ],
    "issuance_source": [
        "request.idleTimeoutSeconds",
        "countEffectiveActiveByActorId(\n                request.actorId,\n                request.idleTimeoutSeconds)",
        "BEGIN IMMEDIATE;",
    ],
    "http_header": [
        "BrowserSessionIdleConfiguration",
        "idleConfiguration_",
    ],
    "http_source": [
        "browser_session_idle_configuration_invalid",
        "request.idleTimeoutSeconds = idleConfiguration_.timeoutSeconds",
    ],
    "lifecycle_gate": [
        "configuration_.browserSessionIdle.valid()",
        "browser_session_idle_configuration_invalid",
        "BrowserSessionIdleConfiguration::LastSeenWriteIntervalSeconds",
    ],
    "server": [
        "configuration.browserSessionIdle.valid()",
        "configuration.browserSessionIdle.timeoutSeconds",
        "BrowserSessionIdleConfiguration::LastSeenWriteIntervalSeconds",
        "configuration.browserSessionIdle)",
    ],
    "packaging": [
        "VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS=0",
        "never extend the absolute browser-session lifetime",
    ],
    "contract": [
        "Browser-Session Idle Expiry",
        "last_seen_at",
        "60-second interval",
        "never extended",
        "physical cleanup or retention",
        "automatic session eviction",
    ],
    "test": [
        "BrowserSessionAuthenticator authenticator",
        "-301 seconds",
        "session_expired",
        "countEffectiveActiveByActorId(\"user-idle\", 300)",
        "browser_session_idle_configuration_invalid",
        "afterThrottled->lastSeenAt == afterTouch->lastSeenAt",
        "afterTouch->expiresAt == absoluteExpiry",
    ],
    "make": [
        "check_browser_session_idle_expiry.py",
        "test-security-browser-session-idle-expiry",
        "test_browser_session_idle_expiry.cpp",
    ],
}


def fail(message: str) -> None:
    print("Browser-session idle-expiry architecture check failed:")
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

    configuration = texts["configuration"]
    if "timeoutSeconds = 300" in configuration:
        fail("compatibility default must remain disabled")

    packaging = texts["packaging"]
    if "VDR_SUITE_BROWSER_SESSION_IDLE_TIMEOUT_SECONDS=300" in packaging:
        fail("packaging must not enable idle expiry by default")

    repository = texts["repository_source"]
    resolved_start = repository.find("findResolvedByTokenId")
    count_start = repository.find("countEffectiveActiveByActorId")
    touch_start = repository.find("touchLastSeenIfDue")
    revoke_start = repository.find("revokeBySessionId", touch_start)
    if min(resolved_start, count_start, touch_start, revoke_start) < 0:
        fail("could not isolate repository idle methods")

    resolved_body = repository[resolved_start:count_start]
    if "updated_at" in resolved_body:
        fail("request-time idle resolution must not use updated_at")
    if "last_seen_at" not in resolved_body:
        fail("request-time idle resolution must use last_seen_at")

    count_body = repository[count_start:touch_start]
    if "UPDATE " in count_body or "DELETE " in count_body:
        fail("effective active count must remain read-only")
    if "last_seen_at" not in count_body:
        fail("effective active count must exclude idle-expired rows")

    touch_body = repository[touch_start:revoke_start]
    if "expires_at" in touch_body:
        fail("activity persistence must not change absolute expiry")
    if "last_seen_at = CURRENT_TIMESTAMP" not in touch_body:
        fail("activity persistence must update only last_seen_at")

    authenticator = texts["authenticator"]
    absolute_expiry_at = authenticator.find("if (record->expired)")
    idle_expiry_at = authenticator.find("if (record->idleExpired)")
    touch_at = authenticator.find("touchLastSeenIfDue")
    grants_at = authenticator.find("findActiveGrantsForActor")
    if not (
        absolute_expiry_at >= 0
        and idle_expiry_at > absolute_expiry_at
        and touch_at > idle_expiry_at
        and grants_at > touch_at
    ):
        fail("activity touch must occur only after effective authentication and before authorization")
    if "expiresAt" in authenticator or "expires_at" in authenticator:
        fail("authenticator must not implement sliding absolute expiry")

    issuance = texts["issuance_source"]
    transaction_at = issuance.find("BEGIN IMMEDIATE;")
    count_at = issuance.find("countEffectiveActiveByActorId")
    insert_at = issuance.find("createSessionCredential")
    if not (transaction_at >= 0 and count_at > transaction_at and insert_at > count_at):
        fail("idle-aware effective count must remain inside the issuance transaction")

    print("Browser-session idle-expiry architecture check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
