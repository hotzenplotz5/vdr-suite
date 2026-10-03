#!/usr/bin/env python3
"""Guard the bounded MU.8A secret-free credential/session metadata slice."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "repo_cpp": ROOT / "core/security/src/HumanAccountCredentialSessionReadRepository.cpp",
    "service_cpp": ROOT / "core/security/src/HumanAccountCredentialSessionReadService.cpp",
    "auth": ROOT / "core/security/include/AuthorizationService.h",
    "gate": ROOT / "core/security/include/SecurityHttpGate.h",
    "runtime_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "runtime_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "api_test": ROOT / "api/rest/tests/test_public_account_security_metadata.cpp",
    "security_test": ROOT / "core/security/tests/test_public_account_security_metadata_security.cpp",
    "server_cpp": ROOT / "core/http/src/TestHttpServer.cpp",
    "client": ROOT / "clients/reference-js/public-v1-client.js",
    "doc": ROOT / "docs/development/post-phase69-mu8a-credential-session-metadata.md",
    "current": ROOT / "docs/CURRENT.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
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


def main():
    for marker in (
        "credential_type <> 'browser-session'",
        "issued_from_credential_id",
        "security_browser_session_credentials",
        "security_sessions",
        "security_credentials",
    ):
        require("repo_cpp", marker)

    for forbidden in (
        "password_hash",
        "session_secret_hash",
        "csrf_secret_hash",
        "token_id",
    ):
        forbid("repo_cpp", forbidden)

    for marker in ("readCredentials", "readSessions", "ActorType::User"):
        require("service_cpp", marker)

    for marker in (
        '"accounts.credentials.view"',
        '"accounts.sessions.view"',
    ):
        require("auth", marker)
        require("gate", marker)

    for marker in (
        "isPublicAccountCredentialResource",
        "isPublicAccountSessionResource",
        "isPublicAccountCredentialRead",
        "isPublicAccountSessionRead",
    ):
        require("gate", marker)

    for marker in (
        "PublicAccountCredentialItem",
        "PublicAccountSessionItem",
        "registerAccountCredentialLookup",
        "registerAccountSessionLookup",
    ):
        require("runtime_h", marker)

    for marker in (
        "publicAccountCredentialPath",
        "publicAccountSessionPath",
        "publicAccountCredentialCollectionResponse",
        "publicAccountSessionCollectionResponse",
        "public-api.accounts-credential-session-metadata",
    ):
        require("runtime_cpp", marker)

    for name in ("runtime_cpp", "client"):
        for forbidden in (
            "passwordHash",
            "sessionSecretHash",
            "csrfSecretHash",
            "tokenId",
            "accounts.credentials.revoke",
        ):
            forbid(name, forbidden)

    for marker in (
        "HumanAccountCredentialSessionReadService",
        "registerAccountCredentialLookup",
        "registerAccountSessionLookup",
        "resetAccountCredentialLookup",
        "resetAccountSessionLookup",
    ):
        require("server_cpp", marker)

    for marker in (
        "getAccountCredentials(options)",
        "getAccountSessions(options)",
    ):
        require("client", marker)

    for marker in (
        "issuedFromCredentialId",
        'headers.at("Allow") == "GET"',
    ):
        require("api_test", marker)

    for marker in (
        '"accounts.credentials.view"',
        '"accounts.sessions.view"',
        '"role.admin"',
        '"backend_scope_denied"',
        "hasDecisionEvent",
    ):
        require("security_test", marker)

    for marker in (
        "MU.8A",
        "GET /api/v1/accounts/{accountId}/credentials",
        "GET /api/v1/accounts/{accountId}/sessions",
        "No credential or session revoke mutation",
        "issuedFromCredentialId",
    ):
        require("doc", marker)

    require(
        "current",
        "MU.8A - Safe credential/session metadata [IMPLEMENTATION CANDIDATE]",
    )
    require(
        "workstream",
        "MU.8A - Safe credential/session metadata [IMPLEMENTATION CANDIDATE]",
    )

    require("make", "test-security-human-account-credential-session-read:")
    require(
        "make",
        "test-security-public-account-security-metadata: "
        "test-security-human-account-credential-session-read",
    )
    forbid(
        "make",
        "$(MAKE) test-security-human-account-credential-session-read",
    )
    occurrences = TEXT["make"].count(
        "python3 tools/check_mu8a_credential_session_metadata.py"
    )
    if occurrences != 2:
        raise AssertionError(
            f"MU.8A guard must be wired exactly twice, found {occurrences}"
        )

    print("MU.8A credential/session metadata contracts passed")
    print("MU8A=IMPLEMENTATION_CANDIDATE")
    print("MU8B_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU8A_GUARD")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.8A metadata check failed:", file=sys.stderr)
        print(f"- {error}", file=sys.stderr)
        raise SystemExit(1)
