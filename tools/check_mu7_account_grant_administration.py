#!/usr/bin/env python3
"""Guard the bounded MU.7 Account grant-administration slice."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "service_h": ROOT / "core/security/include/HumanAccountGrantAdministrationService.h",
    "service_cpp": ROOT / "core/security/src/HumanAccountGrantAdministrationService.cpp",
    "service_test": ROOT / "core/security/tests/test_human_account_grant_administration_service.cpp",
    "auth": ROOT / "core/security/include/AuthorizationService.h",
    "gate": ROOT / "core/security/include/SecurityHttpGate.h",
    "runtime_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "runtime_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "api_test": ROOT / "api/rest/tests/test_public_account_grants.cpp",
    "security_test": ROOT / "core/security/tests/test_public_account_grants_security.cpp",
    "server_h": ROOT / "core/http/include/TestHttpServer.h",
    "server_cpp": ROOT / "core/http/src/TestHttpServer.cpp",
    "client": ROOT / "clients/reference-js/public-v1-client.js",
    "client_test": ROOT / "clients/reference-js/tests/test_public_v1_account_grants_client.js",
    "candidate": ROOT / "docs/development/post-phase69-mu7-account-grant-administration.md",
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
    "mu6d_guard": ROOT / "tools/check_mu6d_account_create_idempotency.py",
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
        "HumanAccountGrantAdministrationStatus",
        "HumanAccountGrantSet",
        "resourceRevision",
        "setGrant",
        "supportedGrant",
    ):
        require("service_h", marker)

    for marker in (
        '"role.admin"',
        '"role.read-only"',
        '"channels.view"',
        '"timers.view"',
        '"media.recording.play"',
        '"media.live.play"',
        '"remote.control"',
        '"osd.view"',
        '"recordings.delete"',
        '"searchtimers.modify"',
        '"broadcast.teletext.view"',
        '"broadcast.hbbtv.view"',
        "EVP_Digest(",
        "EVP_sha256()",
        '"grant-set:"',
        '"grant-set/1\\n"',
        "BEGIN IMMEDIATE;",
        "countUsableAdministratorsExcludingActor",
        '"final_usable_administrator"',
        '"accounts.grants.modify"',
        "grant_already_active",
        "grant_already_absent",
        "grant_set_revision_conflict",
    ):
        require("service_cpp", marker)

    for forbidden in (
        '"authentication.access",',
        '"security.permissions.resolve",',
        '"unmapped.mutation",',
        '"unmapped.browser.mutation",',
        '"accounts.create",',
        '"accounts.modify",',
        '"accounts.grants.view",',
    ):
        forbid("service_cpp", forbidden)

    require(
        "service_cpp",
        'permission == "role.admin" &&\n        backendId == "*"',
    )

    for marker in (
        "initial.grantSet.grants.empty()",
        '"authentication.access"',
        "internalOnly.grantSet.resourceRevision ==",
        "replayEnsure.grantSet.resourceRevision ==",
        "HumanAccountGrantAdministrationStatus::revisionConflict",
        "HumanAccountGrantAdministrationStatus::finalAdministrator",
        "afterAccount.account.revision ==",
        '"unsafe/scope"',
    ):
        require("service_test", marker)

    for marker in (
        'permission == "accounts.grants.modify"',
        'permission == "accounts.grants.view"',
    ):
        require("auth", marker)

    for marker in (
        "isPublicAccountGrantResource",
        "isPublicAccountGrantRead",
        "isPublicAccountGrantMutation",
        'grantReadRequest.permission =\n                "accounts.grants.view"',
        'requestToAuthorize.permission =\n                "accounts.grants.modify"',
        'requestToAuthorize.backendId = "*"',
    ):
        require("gate", marker)

    for marker in (
        "PublicAccountGrantStatus",
        "PublicAccountGrantSetResource",
        "PublicAccountGrantMutationRequest",
        "registerAccountGrantLookup",
        "registerAccountGrantMutation",
    ):
        require("runtime_h", marker)

    for marker in (
        "publicAccountGrantPath",
        "parsePublicAccountGrantMutationBody",
        "publicGrantSetRevision",
        "publicAccountGrantSetResponse",
        "public-api.accounts-grants-administration",
        "Account Grant mutation requires one strong If-Match entity tag.",
        "If-Match does not identify an Account Grant-set revision.",
        "The Account Grant set changed after it was read.",
        "The final usable administrator cannot lose role.admin@*.",
        '"GET, POST"',
    ):
        require("runtime_cpp", marker)

    for forbidden in (
        "sqlite3",
        "SecurityPermissionGrantRepository",
        "HumanAccountGrantAdministrationService",
    ):
        forbid("runtime_cpp", forbidden)

    for marker in (
        "public-api.accounts-grants-administration",
        "statusCode == 304",
        "statusCode == 428",
        "statusCode == 412",
        "statusCode == 409",
        'publicStrongEntityTag("account:7")',
        'headers.at("Allow") ==\n        "GET, POST"',
    ):
        require("api_test", marker)

    for marker in (
        '"accounts.grants.view"',
        '"accounts.grants.modify"',
        '"role.admin"',
        '"backend_scope_denied"',
        '"role_read_only"',
        "includeCsrf",
        'authorizationDecision.backendId ==\n            "*"',
    ):
        require("security_test", marker)

    for marker in (
        "HumanAccountGrantAdministrationService",
        "registerAccountGrantLookup",
        "registerAccountGrantMutation",
        "resetAccountGrantLookup",
        "resetAccountGrantMutation",
    ):
        require("server_cpp", marker)

    require("server_h", "HumanAccountGrantAdministrationService")

    for marker in (
        "function accountGrantPath",
        "getAccountGrants(options)",
        "setAccountGrant(options)",
        "permission: normalizedOptions.permission",
        "backendId: normalizedOptions.backendId",
        "active: normalizedOptions.active",
    ):
        require("client", marker)

    for marker in (
        "client.getAccountGrants",
        "client.setAccountGrant",
        "If-None-Match",
        "If-Match",
        "X-CSRF-Token",
        "revision_conflict",
        "test_public_v1_account_grants_client passed",
    ):
        require("client_test", marker)

    for marker in (
        "# MU.7 — Backend Access / Permission Grant Administration",
        "CANDIDATE — LOCAL ACCEPTANCE PENDING",
        "GET /api/v1/accounts/{accountId}/grants",
        "POST /api/v1/accounts/{accountId}/grants",
        "accounts.grants.view@*",
        "accounts.grants.modify@*",
        "grant-set:<digest>",
        "stale revision + tuple already in requested final state",
        "final usable administrator",
        "MU.8",
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
        require(name, "MU.7")
        require(name, "LOCAL ACCEPTANCE")
        require(name, "MU.8")

    navigation_docs = status_docs + ("development_index",)
    for name in navigation_docs:
        require(name, "MU.7")

    for name in navigation_docs:
        for forbidden in (
            "PR pending",
            "PR PENDING",
            "MU.7 grant administration remains not started",
            "MU.7 - Backend access / permission grant administration [NOT STARTED]",
            "MU.7 grant administration [NOT STARTED]",
        ):
            forbid(name, forbidden)

    require(
        "current",
        "MU.7 - Grant-set read + desired-state grant mutation [CANDIDATE - LOCAL ACCEPTANCE PENDING]",
    )
    require(
        "current",
        "MU.8 - Credential and session administration [NOT STARTED]",
    )
    require(
        "workstream",
        "MU.7 — Backend access / permission grant administration [CANDIDATE — LOCAL ACCEPTANCE PENDING]",
    )
    require("phase_map", "MU.6D Account CREATE/idempotency [DONE - PR #411]")
    require(
        "phase_map",
        "MU.7 grant administration [CANDIDATE - LOCAL ACCEPTANCE PENDING]",
    )
    require(
        "development_index",
        "MU.7 Backend Access / Permission Grant Administration",
    )

    require("mu6d_guard", "MU6D=COMPLETED")
    require(
        "mu6d_guard",
        "MU7_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU6D_GUARD",
    )

    require("make", "test-security-human-account-grant-administration:")
    require("make", "test-security-public-account-grants:")
    occurrences = TEXT["make"].count(
        "python3 tools/check_mu7_account_grant_administration.py"
    )
    if occurrences != 2:
        raise AssertionError(
            f"MU.7 guard must be wired exactly twice, found {occurrences}"
        )

    for name in ("runtime_cpp", "service_cpp", "server_cpp", "client"):
        for forbidden in (
            "accounts.credentials.revoke",
            "accounts.sessions.revoke",
            "password.reset",
            "credential.rotate",
        ):
            forbid(name, forbidden)

    print("MU.7 Account grant administration contracts passed")
    print("MU6=COMPLETED")
    print("MU7=CANDIDATE_LOCAL_ACCEPTANCE_PENDING")
    print("MU8=NOT_STARTED")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.7 grant administration check failed:", file=sys.stderr)
        print(f"- {error}", file=sys.stderr)
        raise SystemExit(1)
