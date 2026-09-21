#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "configuration": ROOT / "core/security/include/SecurityConfiguration.h",
    "credential_header": ROOT / "core/security/include/BrowserSessionCredentialRepository.h",
    "credential_source": ROOT / "core/security/src/BrowserSessionRetentionRepository.cpp",
    "identity_header": ROOT / "core/security/include/SecurityIdentityRepository.h",
    "identity_source": ROOT / "core/security/src/SecurityIdentityRetentionRepository.cpp",
    "service_header": ROOT / "core/security/include/BrowserSessionRetentionService.h",
    "service_source": ROOT / "core/security/src/BrowserSessionRetentionService.cpp",
    "server_header": ROOT / "core/http/include/TestHttpServer.h",
    "server_source": ROOT / "core/http/src/TestHttpServer.cpp",
    "packaging": ROOT / "packaging/systemd/vdr-suite-daemon.default",
    "contract": ROOT / "docs/development/phase-62-slice-2w-browser-session-retention-cleanup.md",
    "test": ROOT / "core/security/tests/test_browser_session_retention_cleanup.cpp",
    "make": ROOT / "mk/security-sources.mk",
}

REQUIRED = {
    "configuration": [
        "BrowserSessionRetentionConfiguration",
        "seconds = 0",
        "MinimumRetentionSeconds = 86400",
        "MaximumRetentionSeconds = 31536000",
        "BatchSize = 256",
        "VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS",
        "parseBrowserSessionRetention",
    ],
    "credential_header": [
        "TerminalBrowserSessionCandidate",
        "findTerminalRetentionCandidates",
        "remainsTerminalRetentionCandidate",
        "deleteTerminalRetentionCandidate",
    ],
    "credential_source": [
        "MaximumRetentionBatchSize = 256",
        "revoked_at <> ''",
        "expires_at <=",
        "last_seen_at <=",
        "(?1 + ?2)",
        "ORDER BY terminal_at, token_id",
        "LIMIT ?3",
        "remainsTerminalRetentionCandidate",
        "DELETE FROM security_browser_session_credentials",
    ],
    "identity_header": [
        "deleteSessionIfUnreferenced",
        "deleteBrowserSessionCredentialIfUnreferenced",
    ],
    "identity_source": [
        "DELETE FROM security_sessions",
        "DELETE FROM security_credentials",
        "credential_type = 'browser-session'",
        "NOT EXISTS",
        "security_browser_session_credentials",
    ],
    "service_header": [
        "BrowserSessionRetentionService",
        "BrowserSessionRetentionConfiguration",
        "BrowserSessionIdleConfiguration",
    ],
    "service_source": [
        "BEGIN IMMEDIATE;",
        "BrowserSessionRetentionConfiguration::BatchSize",
        "findTerminalRetentionCandidates",
        "remainsTerminalRetentionCandidate",
        "appendCleanupEvent(candidate)",
        "deleteTerminalRetentionCandidate(candidate)",
        "security,lifecycle,maintenance",
        "operation.succeeded",
        "system-maintenance",
        "browser.session.cleanup",
        "completed",
        "browser_session_retention_elapsed",
        "deleted",
    ],
    "server_header": [
        "BrowserSessionRetentionService.h",
        "browserSessionRetentionService_",
    ],
    "server_source": [
        "configuration.browserSessionLifetime.valid()",
        "configuration.browserSessionConcurrency.valid()",
        "configuration.browserSessionIdle.valid()",
        "configuration.browserSessionRetention.valid()",
        "std::make_unique<BrowserSessionRetentionService>",
        "configuration.browserSessionRetention",
        "securityReady_ = true",
    ],
    "packaging": [
        "VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS=0",
        "fixed startup batch of at most 256",
    ],
    "contract": [
        "Browser-Session Terminal Retention Cleanup",
        "oldest terminal time first",
        "256",
        "BEGIN IMMEDIATE",
        "browser.session.cleanup",
        "browser_session_retention_elapsed",
    ],
    "test": [
        "testDisabledPolicy",
        "testEligibilityAndPreservation",
        "testIdleEligibility",
        "testOwnedIdentityDeletion",
        "testBoundedDeterministicBatch",
        "testAuditFailureRollback",
        "testSqlFailureRollsBackWholeBatch",
        "events.size() == 2",
        "listAll().size() == 256",
    ],
    "make": [
        "BrowserSessionRetentionRepository.cpp",
        "SecurityIdentityRetentionRepository.cpp",
        "BrowserSessionRetentionService.cpp",
        "check_browser_session_retention_cleanup.py",
        "test-security-browser-session-retention-cleanup",
        "test_browser_session_retention_cleanup.cpp",
    ],
}


def fail(message: str) -> None:
    print("Browser-session retention architecture check failed:")
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
    if "seconds = 86400" in configuration:
        fail("compatibility default must remain disabled")

    packaging = texts["packaging"]
    if "VDR_SUITE_BROWSER_SESSION_RETENTION_SECONDS=86400" in packaging:
        fail("packaging must not enable retention cleanup by default")

    service = texts["service_source"]
    transaction_at = service.find("BEGIN IMMEDIATE;")
    selection_at = service.find("findTerminalRetentionCandidates")
    recheck_at = service.find("remainsTerminalRetentionCandidate")
    audit_at = service.find("appendCleanupEvent(candidate)")
    verifier_delete_at = service.find("deleteTerminalRetentionCandidate(candidate)")
    session_delete_at = service.find("deleteSessionIfUnreferenced")
    credential_delete_at = service.find(
        "deleteBrowserSessionCredentialIfUnreferenced")
    commit_at = service.find("transaction.commit()")
    if not (
        transaction_at >= 0
        and selection_at > transaction_at
        and recheck_at > selection_at
        and audit_at > recheck_at
        and verifier_delete_at > audit_at
        and session_delete_at > verifier_delete_at
        and credential_delete_at > session_delete_at
        and commit_at > credential_delete_at
    ):
        fail("cleanup ordering must remain select, recheck, audit, owned deletes, commit")

    for forbidden in (
        "std::thread",
        "scheduler",
        "handleRequest",
        "setInterval",
        "while (true)",
    ):
        if forbidden in service:
            fail(f"retention service contains forbidden runtime mechanism {forbidden!r}")

    append_start = service.find("appendCleanupEvent")
    opaque_start = service.find("opaqueId", append_start)
    if append_start < 0 or opaque_start < 0:
        fail("could not isolate cleanup accountability body")
    append_body = service[append_start:opaque_start]
    for secret_marker in (
        "sessionSecret",
        "csrfSecret",
        "passwordHash",
        "Authorization",
        "Cookie",
        "getenv",
    ):
        if secret_marker in append_body:
            fail(f"cleanup accountability exposes secret source {secret_marker!r}")

    identity = texts["identity_source"]
    for forbidden_delete in (
        "DELETE FROM security_actors",
        "DELETE FROM security_devices",
        "DELETE FROM accountability_events",
        "DELETE FROM security_permission_grants",
        "DELETE FROM security_role",
        "DELETE FROM credential_verifiers",
    ):
        if forbidden_delete in identity:
            fail(f"identity cleanup broadens deletion to {forbidden_delete!r}")

    combined_cleanup_sql = texts["credential_source"] + identity
    if "ON DELETE CASCADE" in combined_cleanup_sql:
        fail("retention cleanup must not introduce cascade deletion")

    server = texts["server_source"]
    validation_at = server.find("configuration.browserSessionRetention.valid()")
    cleanup_at = server.find("browserSessionRetentionService_->cleanup")
    ready_at = server.find("securityReady_ = true")
    request_at = server.find("TestHttpServer::handleRequest")
    if not (
        validation_at >= 0
        and cleanup_at > validation_at
        and ready_at > cleanup_at
        and request_at > ready_at
    ):
        fail("cleanup must run once during startup after validation and before readiness")
    if server.count("browserSessionRetentionService_->cleanup") != 1:
        fail("startup must own exactly one retention cleanup call")

    print("Browser-session retention architecture check passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())