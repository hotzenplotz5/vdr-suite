#!/usr/bin/env python3
"""Guard the bounded MU.8C human-password Credential revoke slice."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "repo_h": ROOT / "core/security/include/HumanAccountCredentialSessionReadRepository.h",
    "repo_cpp": ROOT / "core/security/src/HumanAccountCredentialSessionReadRepository.cpp",
    "read_h": ROOT / "core/security/include/HumanAccountCredentialSessionReadService.h",
    "read_cpp": ROOT / "core/security/src/HumanAccountCredentialSessionReadService.cpp",
    "admin_repo_h": ROOT / "core/security/include/HumanAccountAdministrationRepository.h",
    "admin_repo_cpp": ROOT / "core/security/src/HumanAccountAdministrationRepository.cpp",
    "admin_h": ROOT / "core/security/include/HumanAccountCredentialAdministrationService.h",
    "admin_cpp": ROOT / "core/security/src/HumanAccountCredentialAdministrationService.cpp",
    "auth": ROOT / "core/security/include/AuthorizationService.h",
    "gate": ROOT / "core/security/include/SecurityHttpGate.h",
    "runtime_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "runtime_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "server_h": ROOT / "core/http/include/TestHttpServer.h",
    "server_cpp": ROOT / "core/http/src/TestHttpServer.cpp",
    "service_test": ROOT / "core/security/tests/test_human_account_credential_administration_service.cpp",
    "api_test": ROOT / "api/rest/tests/test_public_account_credential_revoke.cpp",
    "security_test": ROOT / "core/security/tests/test_public_account_credential_revoke_security.cpp",
    "client": ROOT / "clients/reference-js/public-v1-client.js",
    "client_test": ROOT / "clients/reference-js/tests/test_public_v1_account_credential_revoke_client.js",
    "mu8b_guard": ROOT / "tools/check_mu8b_session_revoke.py",
    "doc": ROOT / "docs/development/post-phase69-mu8c-credential-revoke.md",
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
    for marker in ("resourceRevision",):
        require("repo_h", marker)

    for marker in (
        "credential-lifecycle/1",
        "credentialLifecycleRevision(",
        "EVP_sha256",
        "revoked_at",
    ):
        require("repo_cpp", marker)

    for marker in (
        "credentialNotFound",
        "readCredential(",
        "HumanAccountCredentialItemReadResult",
    ):
        require("read_h", marker)
        require("read_cpp", marker)

    require(
        "admin_repo_h",
        "wouldRevokeFinalUsableAdministrator(",
    )
    for marker in (
        "wouldRevokeFinalUsableAdministrator(",
        "credential.credential_id <> ?1",
        "security_basic_credential_verifiers",
        "role.admin",
        "human-password",
    ):
        require("admin_repo_cpp", marker)

    for marker in (
        "HumanAccountCredentialAdministrationStatus",
        "expectedResourceRevision",
        "finalAdministrator",
        "revokedBrowserSessions",
    ):
        require("admin_h", marker)

    for marker in (
        "BEGIN IMMEDIATE",
        'credentialType != "human-password"',
        "wouldRevokeFinalUsableAdministrator",
        "findByIssuedFromCredentialId",
        "revokeInActiveTransaction",
        '"accounts.credentials.revoke"',
        '"human-account.credential.revoke"',
        '"credential_revision_conflict"',
        '"final_usable_administrator"',
        '"credential_revoked"',
        '"credential_already_terminal"',
    ):
        require("admin_cpp", marker)

    require("auth", 'permission == "accounts.credentials.revoke"')
    for marker in (
        "isPublicAccountCredentialItemResource",
        "isPublicAccountCredentialMutation",
        '"accounts.credentials.revoke"',
    ):
        require("gate", marker)

    for marker in (
        "PublicAccountCredentialResource",
        "PublicAccountCredentialMutationRequest",
        "registerAccountCredentialItemLookup",
        "registerAccountCredentialMutation",
    ):
        require("runtime_h", marker)

    for marker in (
        "publicAccountCredentialItemPath",
        "publicCredentialLifecycleRevision",
        "publicAccountCredentialResourceResponse",
        "public-api.accounts-credential-revoke",
        "Account Credential revoke requires one strong If-Match entity tag.",
        "The Account Credential lifecycle changed after it was read.",
        "The final usable administrator credential cannot be revoked.",
    ):
        require("runtime_cpp", marker)

    public_scope = between(
        "runtime_cpp",
        "ApiResponse publicAccountCredentialResourceResponse(",
        "ApiResponse publicAccountCredentialCollectionResponse(",
    )
    for forbidden in (
        "passwordHash",
        "password_hash",
        "sessionSecretHash",
        "csrfSecretHash",
        "tokenId",
        "browserCredentialId",
        "resourceRevision\":",
    ):
        if forbidden in public_scope:
            raise AssertionError(
                f"public Credential item leaks forbidden marker: {forbidden}"
            )

    for marker in (
        "HumanAccountCredentialAdministrationService",
        "humanAccountCredentialAdministrationService_",
    ):
        require("server_h", marker)

    for marker in (
        "registerAccountCredentialItemLookup",
        "registerAccountCredentialMutation",
        "resetAccountCredentialItemLookup",
        "resetAccountCredentialMutation",
        "HumanAccountCredentialAdministrationService",
    ):
        require("server_cpp", marker)

    for marker in (
        "credential-lifecycle:",
        "finalAdministrator",
        "credential-human-b",
        "revokedBrowserSessions == 1U",
        "credential_revision_conflict",
        "final_usable_administrator",
        "credential_already_terminal",
    ):
        require("service_test", marker)

    for marker in (
        "statusCode == 428",
        "statusCode == 412",
        "statusCode == 409",
        '"GET, POST"',
        "public-api.accounts-credential-revoke",
        "resourceRevision",
    ):
        require("api_test", marker)

    for marker in (
        '"accounts.credentials.revoke"',
        '"role.admin"',
        '"role.read-only"',
        "includeCsrf",
        '"backend_scope_denied"',
    ):
        require("security_test", marker)

    for marker in (
        "getAccountCredential(options)",
        "revokeAccountCredential(options)",
    ):
        require("client", marker)

    for marker in (
        "getAccountCredential",
        "revokeAccountCredential",
        "If-Match",
        "revision_conflict",
        "operation_conflict",
    ):
        require("client_test", marker)

    for marker in (
        "MU.8C",
        "GET /api/v1/accounts/{accountId}/credentials/{credentialId}",
        "POST /api/v1/accounts/{accountId}/credentials/{credentialId}",
        "accounts.credentials.revoke@*",
        "final usable administrator",
        "issuer-session",
        "human-password",
    ):
        require("doc", marker)

    require(
        "current",
        "MU.8C merged evidence: [MU.8C Human-password Credential Revoke]",
    )
    require(
        "doc",
        "Status: **IMPLEMENTATION MERGED — PR #417 / FOCUSED YAVDR PASS / HOSTED CI GREEN**",
    )
    require(
        "workstream",
        "MU.8C - Human-password Credential Revoke [IMPLEMENTATION MERGED - PR #417 / FOCUSED YAVDR PASS / HOSTED CI GREEN]",
    )
    require("phase_map", "MU.8C")

    require(
        "mu8b_guard",
        "MU8C_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU8B_GUARD",
    )

    require(
        "make",
        "test-security-human-account-credential-administration:",
    )
    require(
        "make",
        "test-security-public-account-credential-revoke: "
        "test-security-human-account-credential-administration",
    )
    occurrences = TEXT["make"].count(
        "python3 tools/check_mu8c_credential_revoke.py"
    )
    if occurrences != 2:
        raise AssertionError(
            f"MU.8C guard must be wired exactly twice, found {occurrences}"
        )

    print("MU.8C Credential revoke contracts passed")
    print("MU8C=IMPLEMENTATION_MERGED_PR_417_HOSTED_CI_GREEN")
    print("MU9_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU8C_GUARD")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.8C Credential revoke check failed:", file=sys.stderr)
        print(f"- {error}", file=sys.stderr)
        raise SystemExit(1)
