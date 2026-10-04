#!/usr/bin/env python3
"""Guard the bounded MU.8B revisioned Session revoke slice."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "repo_h": ROOT / "core/security/include/HumanAccountCredentialSessionReadRepository.h",
    "repo_cpp": ROOT / "core/security/src/HumanAccountCredentialSessionReadRepository.cpp",
    "read_h": ROOT / "core/security/include/HumanAccountCredentialSessionReadService.h",
    "read_cpp": ROOT / "core/security/src/HumanAccountCredentialSessionReadService.cpp",
    "admin_h": ROOT / "core/security/include/HumanAccountSessionAdministrationService.h",
    "admin_cpp": ROOT / "core/security/src/HumanAccountSessionAdministrationService.cpp",
    "lifecycle_h": ROOT / "core/security/include/BrowserSessionLifecycleService.h",
    "auth": ROOT / "core/security/include/AuthorizationService.h",
    "gate": ROOT / "core/security/include/SecurityHttpGate.h",
    "runtime_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "runtime_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "server_h": ROOT / "core/http/include/TestHttpServer.h",
    "server_cpp": ROOT / "core/http/src/TestHttpServer.cpp",
    "service_test": ROOT / "core/security/tests/test_human_account_session_administration_service.cpp",
    "api_test": ROOT / "api/rest/tests/test_public_account_session_revoke.cpp",
    "security_test": ROOT / "core/security/tests/test_public_account_session_revoke_security.cpp",
    "client": ROOT / "clients/reference-js/public-v1-client.js",
    "client_test": ROOT / "clients/reference-js/tests/test_public_v1_account_session_revoke_client.js",
    "mu8a_guard": ROOT / "tools/check_mu8a_credential_session_metadata.py",
    "doc": ROOT / "docs/development/post-phase69-mu8b-session-revoke.md",
    "current": ROOT / "docs/CURRENT.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
    "phase_map": ROOT / "docs/planning/phase-map.md",
    "make": ROOT / "mk/security-sources.mk",
}

TEXT = {}
for name, path in FILES.items():
    if not path.exists():
        raise SystemExit(f"missing file: {path.relative_to(ROOT)}")
    TEXT[name] = path.read_text(encoding="utf-8")


def require(name, marker):
    if marker not in TEXT[name]:
        raise AssertionError(f"{name} missing marker: {marker}")


def forbid(name, marker):
    if marker in TEXT[name]:
        raise AssertionError(f"{name} contains forbidden marker: {marker}")


def between(name, start_marker, end_marker):
    text = TEXT[name]
    start = text.find(start_marker)
    if start < 0:
        raise AssertionError(f"{name} missing scoped start marker: {start_marker}")
    end = text.find(end_marker, start + len(start_marker))
    if end < 0:
        raise AssertionError(f"{name} missing scoped end marker: {end_marker}")
    return text[start:end]


def main():
    for marker in (
        "browserCredentialId",
        "resourceRevision",
    ):
        require("repo_h", marker)

    for marker in (
        "session-lifecycle/1",
        "EVP_sha256",
        "browser.credential_id",
        "issuing_credential.revoked_at",
    ):
        require("repo_cpp", marker)

    revision_scope = between(
        "repo_cpp",
        "std::string sessionLifecycleRevision(",
        "HumanAccountCredentialSessionReadRepository::",
    )
    for forbidden in ("lastSeenAt", "last_seen_at"):
        if forbidden in revision_scope:
            raise AssertionError(
                "session lifecycle revision must ignore passive lastSeen activity"
            )

    for marker in (
        "sessionNotFound",
        "readSession(",
    ):
        require("read_h", marker)
        require("read_cpp", marker)

    for marker in (
        "HumanAccountSessionAdministrationStatus",
        "expectedResourceRevision",
        "revisionConflict",
    ):
        require("admin_h", marker)

    for marker in (
        "BEGIN IMMEDIATE",
        "revokeInActiveTransaction",
        "session_already_terminal",
        "session_revision_conflict",
        "session_revoked",
        '"accounts.sessions.revoke"',
        '"human-account.session.revoke"',
    ):
        require("admin_cpp", marker)

    require("lifecycle_h", "bool revokeInActiveTransaction(")

    require("auth", 'permission == "accounts.sessions.revoke"')
    for marker in (
        "isPublicAccountSessionItemResource",
        "isPublicAccountSessionMutation",
        '"accounts.sessions.revoke"',
    ):
        require("gate", marker)

    for marker in (
        "PublicAccountSessionResource",
        "PublicAccountSessionMutationRequest",
        "registerAccountSessionItemLookup",
        "registerAccountSessionMutation",
    ):
        require("runtime_h", marker)

    for marker in (
        "publicAccountSessionItemPath",
        "publicSessionLifecycleRevision",
        "publicAccountSessionResourceResponse",
        "public-api.accounts-session-revoke",
        "Account Session revoke requires one strong If-Match entity tag.",
        "The Account Session lifecycle changed after it was read.",
    ):
        require("runtime_cpp", marker)

    for name in ("runtime_cpp", "client"):
        for forbidden in (
            "passwordHash",
            "sessionSecretHash",
            "csrfSecretHash",
            "tokenId",
            "browserCredentialId",
            "accounts.credentials.revoke",
        ):
            forbid(name, forbidden)

    for marker in (
        "HumanAccountSessionAdministrationService",
        "humanAccountSessionAdministrationService_",
    ):
        require("server_h", marker)

    for marker in (
        "registerAccountSessionItemLookup",
        "registerAccountSessionMutation",
        "resetAccountSessionItemLookup",
        "resetAccountSessionMutation",
        "HumanAccountSessionAdministrationService",
    ):
        require("server_cpp", marker)

    for marker in (
        "last_seen_at = '2098-01-01 00:00:00'",
        "session_revision_conflict",
        "session_already_terminal",
        "credential-browser",
    ):
        require("service_test", marker)

    for marker in (
        "statusCode == 428",
        "statusCode == 412",
        '"GET, POST"',
        "public-api.accounts-session-revoke",
        "browserCredentialId",
    ):
        require("api_test", marker)

    for marker in (
        '"accounts.sessions.revoke"',
        '"role.admin"',
        '"role.read-only"',
        "includeCsrf",
        '"backend_scope_denied"',
    ):
        require("security_test", marker)

    for marker in (
        "getAccountSession(options)",
        "revokeAccountSession(options)",
    ):
        require("client", marker)

    for marker in (
        "getAccountSession",
        "revokeAccountSession",
        "If-Match",
        "revision_conflict",
    ):
        require("client_test", marker)

    require(
        "doc",
        "Status: **IMPLEMENTATION MERGED — PR #416 / FOCUSED YAVDR PASS / HOSTED CI GREEN**",
    )

    for marker in (
        "MU.8B",
        "GET /api/v1/accounts/{accountId}/sessions/{sessionId}",
        "POST /api/v1/accounts/{accountId}/sessions/{sessionId}",
        "accounts.sessions.revoke@*",
        "lastSeenAt",
        "already-terminal",
        "accounts.credentials.revoke",
    ):
        require("doc", marker)

    require(
        "current",
        "MU.8B merged evidence: [MU.8B Session Revoke]",
    )
    require(
        "workstream",
        "MU.8B - Session Revoke [IMPLEMENTATION MERGED - PR #416 / FOCUSED YAVDR PASS / HOSTED CI GREEN]",
    )
    require("phase_map", "MU.8B")

    require(
        "mu8a_guard",
        "MU8B_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU8A_GUARD",
    )

    require("make", "test-security-human-account-session-administration:")
    require(
        "make",
        "test-security-public-account-session-revoke: "
        "test-security-human-account-session-administration",
    )
    occurrences = TEXT["make"].count(
        "python3 tools/check_mu8b_session_revoke.py"
    )
    if occurrences != 2:
        raise AssertionError(
            f"MU.8B guard must be wired exactly twice, found {occurrences}"
        )

    print("MU.8B Session revoke contracts passed")
    print("MU8B=IMPLEMENTATION_MERGED_PR_416_HOSTED_CI_GREEN")
    print("MU8C_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU8B_GUARD")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.8B Session revoke check failed:", file=sys.stderr)
        print(f"- {error}", file=sys.stderr)
        raise SystemExit(1)
