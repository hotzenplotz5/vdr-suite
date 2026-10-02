#!/usr/bin/env python3
"""Guard the bounded MU.6D Atomic Account CREATE + durable idempotency slice."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "repo_h": ROOT / "core/security/include/HumanAccountCreationRepository.h",
    "repo_cpp": ROOT / "core/security/src/HumanAccountCreationRepository.cpp",
    "service_h": ROOT / "core/security/include/HumanAccountCreationService.h",
    "service_cpp": ROOT / "core/security/src/HumanAccountCreationService.cpp",
    "runtime_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "runtime_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "auth": ROOT / "core/security/include/AuthorizationService.h",
    "gate": ROOT / "core/security/include/SecurityHttpGate.h",
    "server_h": ROOT / "core/http/include/TestHttpServer.h",
    "server_cpp": ROOT / "core/http/src/TestHttpServer.cpp",
    "service_test": ROOT / "core/security/tests/test_human_account_creation_service.cpp",
    "api_test": ROOT / "api/rest/tests/test_public_account_collection.cpp",
    "security_test": ROOT / "core/security/tests/test_public_account_collection_security.cpp",
    "client": ROOT / "clients/reference-js/public-v1-client.js",
    "client_test": ROOT / "clients/reference-js/tests/test_public_v1_account_create_client.js",
    "candidate": ROOT / "docs/development/post-phase69-mu6d-account-create-idempotency.md",
    "current": ROOT / "docs/CURRENT.md",
    "current_status": ROOT / "docs/development/current-status.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
    "roadmap": ROOT / "docs/planning/roadmap.md",
    "phase_map": ROOT / "docs/planning/phase-map.md",
    "handoff": ROOT / "docs/NEW-CHAT-HANDOFF.md",
    "identity_foundation": ROOT / "docs/architecture/security-identity-foundation.md",
    "gap_matrix": ROOT / "docs/planning/architecture-audit-gap-matrix.md",
    "dependency_map": ROOT / "docs/planning/implementation-dependency-map.md",
    "root_roadmap": ROOT / "ROADMAP.md",
    "planning_index": ROOT / "docs/planning/index.md",
    "development_index": ROOT / "docs/development/index.md",
    "make": ROOT / "mk/security-sources.mk",
}


def read(name):
    path = FILES[name]
    if not path.exists():
        raise AssertionError(f"missing file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


TEXT = {name: read(name) for name in FILES}


def require(name, marker):
    if marker not in TEXT[name]:
        raise AssertionError(
            f"{FILES[name].relative_to(ROOT)} missing marker: {marker}"
        )


def forbid(name, marker):
    if marker in TEXT[name]:
        raise AssertionError(
            f"{FILES[name].relative_to(ROOT)} contains forbidden marker: {marker}"
        )


def main():
    for marker in (
        "security_human_account_create_idempotency",
        "PRIMARY KEY(actor_id, idempotency_key)",
        "login_name",
        "display_name",
        "account_id",
        "insertInActiveTransaction",
    ):
        require("repo_cpp", marker)

    for name in ("repo_h", "repo_cpp"):
        for forbidden in (
            "password",
            "password_hash",
            "passwordHash",
            "credential_id",
            "credentialId",
        ):
            forbid(name, forbidden)

    for marker in (
        "HumanAccountCreationStatus",
        "HumanAccountCreationRequest",
        "HumanAccountCreationResult",
        "clearSecrets",
    ):
        require("service_h", marker)

    for marker in (
        "BEGIN IMMEDIATE",
        "creationRepository_.find",
        "credentialVerifierRepository_.findByLogin",
        "ensureHumanCredentialInActiveTransaction",
        "ensureAccountInActiveTransaction",
        "ensureVerifier",
        "insertInActiveTransaction",
        "security.human-account.create",
        'event.permission = "accounts.create"',
        'event.backendId = "*"',
        'crypt_gensalt_rn(',
        '"$y$"',
        "RequestSecretGuard",
    ):
        require("service_cpp", marker)

    replay_index = TEXT["service_cpp"].find("creationRepository_.find")
    hash_index = TEXT["service_cpp"].find(
        "std::string passwordHash =\n        hashHumanPassword"
    )
    if replay_index < 0 or hash_index < 0 or replay_index >= hash_index:
        raise AssertionError(
            "idempotency replay lookup must happen before password hashing"
        )

    for forbidden in (
        "SecurityPermissionGrantRepository",
        "ensureGrant(",
        '"role.admin"',
        '"role.read-only"',
    ):
        forbid("service_cpp", forbidden)

    for marker in (
        "PublicAccountCreateStatus",
        "PublicAccountCreateRequest",
        "PublicAccountCreateResult",
        "registerAccountCreate",
        "resetAccountCreate",
        "accountCreateConfigured",
    ):
        require("runtime_h", marker)

    for marker in (
        "parsePublicAccountCreateBody",
        "public-api.accounts-create",
        "Idempotency-Key is required for Account CREATE",
        "idempotency_conflict",
        'response.statusCode = 201',
        'response.headers["Location"] = accountPath',
        "createRequest.password.begin()",
        "createRequest.password.clear()",
        '"GET, POST"',
    ):
        require("runtime_cpp", marker)

    for forbidden in (
        "sqlite3",
        "HumanAccountCreationRepository",
        "CredentialVerifierRepository",
    ):
        forbid("runtime_cpp", forbidden)

    require("auth", 'permission == "accounts.create"')
    for marker in (
        "isPublicAccountCreate",
        'requestToAuthorize.permission = "accounts.create"',
        'requestToAuthorize.action = "accounts.create"',
        'requestToAuthorize.backendId = "*"',
    ):
        require("gate", marker)

    for marker in (
        "HumanAccountCreationRepository",
        "HumanAccountCreationService",
        "registerAccountCreate",
        "resetAccountCreate",
    ):
        require("server_cpp", marker)

    for marker in (
        "HumanAccountCreationRepository",
        "HumanAccountCreationService",
    ):
        require("server_h", marker)

    for marker in (
        "HumanAccountCreationStatus::success",
        "HumanAccountCreationStatus::replayed",
        "HumanAccountCreationStatus::idempotencyConflict",
        "HumanAccountCreationStatus::loginConflict",
        "createdGrants.grants.empty()",
        "different-retry-password",
        "fixture.entropyCounter == entropyAfterCreate",
        "fixture.accounts.listAll().accounts.size() == 2U",
    ):
        require("service_test", marker)

    for marker in (
        "PublicAccountCreateStatus::created",
        "PublicAccountCreateStatus::replayed",
        "public-api.accounts-create",
        "idem-account-create-1",
        "idempotency_conflict",
        'created.statusCode == 201',
        'created.body.find("password") == std::string::npos',
    ):
        require("api_test", marker)

    for marker in (
        "browserCreate",
        '"accounts.create"',
        '"backend_scope_denied"',
        '"role_read_only"',
        "includeCsrf",
    ):
        require("security_test", marker)

    for marker in (
        "function requestAccountCreate",
        "createAccount(options)",
        "headers['Idempotency-Key']",
        "'/api/v1/accounts'",
    ):
        require("client", marker)

    for marker in (
        "client.createAccount",
        "X-CSRF-Token",
        "Idempotency-Key",
        "idempotency_conflict",
        "test_public_v1_account_create_client passed",
    ):
        require("client_test", marker)

    for marker in (
        "# MU.6D — Atomic Account CREATE + Durable Idempotency",
        "CANDIDATE — LOCAL ACCEPTANCE PENDING",
        "POST /api/v1/accounts",
        "accounts.create@*",
        "yescrypt",
        "No role or backend permission grant is created",
        "same actor + same key + same non-secret fields",
        "MU.7",
    ):
        require("candidate", marker)

    status_docs = (
        "current",
        "current_status",
        "workstream",
        "roadmap",
        "phase_map",
        "handoff",
        "identity_foundation",
        "gap_matrix",
        "dependency_map",
        "root_roadmap",
        "planning_index",
    )
    for name in status_docs:
        require(name, "MU.6D")
        require(name, "LOCAL ACCEPTANCE")

    navigation_docs = status_docs + ("development_index",)
    for name in navigation_docs:
        require(name, "MU.6D")

    for name in navigation_docs:
        for forbidden in (
            "MU.6C public lifecycle mutation is a CANDIDATE",
            "MU.6D Account CREATE remains not started",
            "MU.6D - Account CREATE [NOT STARTED]",
        ):
            forbid(name, forbidden)

    require("current", "MU.6C - Public display-name / activate / deactivate [COMPLETED]")
    require(
        "current",
        "MU.6D - Atomic Account CREATE + durable idempotency [CANDIDATE - LOCAL ACCEPTANCE PENDING]",
    )
    require("current_status", "MU.7 - Backend access / permission grant administration [NOT STARTED]")
    require("workstream", "MU.6D Atomic Account CREATE + durable idempotency       [CANDIDATE - LOCAL ACCEPTANCE PENDING]")
    require("phase_map", "MU.6C public lifecycle mutation [DONE]")
    require("phase_map", "MU.7 grant administration [NOT STARTED]")
    require("development_index", "MU.6D Atomic Account CREATE + Durable Idempotency")

    require("make", "test-security-human-account-creation")
    occurrences = TEXT["make"].count(
        "python3 tools/check_mu6d_account_create_idempotency.py"
    )
    if occurrences != 2:
        raise AssertionError(
            f"MU.6D guard must be wired exactly twice, found {occurrences}"
        )

    for name in (
        "runtime_cpp",
        "service_cpp",
        "server_cpp",
        "client",
    ):
        for forbidden in (
            "accounts.grants.modify",
            "accounts.credentials.revoke",
            "accounts.sessions.revoke",
        ):
            forbid(name, forbidden)

    print("MU.6D Atomic Account CREATE + durable idempotency contracts passed")
    print("MU6=IN_PROGRESS")
    print("MU6A=DONE")
    print("MU6B=DONE")
    print("MU6C=DONE")
    print("MU6D=CANDIDATE_LOCAL_ACCEPTANCE_PENDING")
    print("MU7=NOT_STARTED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.6D Account CREATE check failed:", file=sys.stderr)
        print(f"- {error}", file=sys.stderr)
        raise SystemExit(1)
