#!/usr/bin/env python3
"""Guard the bounded MU.7A Account backend-access grant administration slice."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "service_h": ROOT / "core/security/include/HumanAccountGrantAdministrationService.h",
    "service_cpp": ROOT / "core/security/src/HumanAccountGrantAdministrationService.cpp",
    "auth": ROOT / "core/security/include/AuthorizationService.h",
    "gate": ROOT / "core/security/include/SecurityHttpGate.h",
    "runtime_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "runtime_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "server_h": ROOT / "core/http/include/TestHttpServer.h",
    "server_cpp": ROOT / "core/http/src/TestHttpServer.cpp",
    "service_test": ROOT / "core/security/tests/test_human_account_grant_administration_service.cpp",
    "api_test": ROOT / "api/rest/tests/test_public_account_grants.cpp",
    "gate_test": ROOT / "core/security/tests/test_public_account_grant_security.cpp",
    "candidate": ROOT / "docs/development/post-phase69-mu7a-backend-access-grant-administration.md",
    "current": ROOT / "docs/CURRENT.md",
    "status": ROOT / "docs/development/current-status.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
    "roadmap": ROOT / "docs/planning/roadmap.md",
    "phase_map": ROOT / "docs/planning/phase-map.md",
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
        "HumanAccountGrantResource",
        "resourceRevision",
        "setGrant",
    ):
        require("service_h", marker)

    for marker in (
        'return permission == "channels.view"',
        "concreteBackendId",
        "findActiveGrantsForActor",
        "ensureGrant(",
        "revokeGrant(",
        "BEGIN IMMEDIATE",
        "grant_set_revision_conflict",
        "grant_already_active",
        "grant_already_revoked",
        "security.human-account.grant-administration",
        'event.permission = "accounts.grants.modify"',
    ):
        require("service_cpp", marker)

    for forbidden in (
        '"role.admin"',
        '"role.read-only"',
        "CREATE TABLE",
        "password",
        "credential",
        "session",
    ):
        forbid("service_cpp", forbidden)

    require("auth", 'permission == "accounts.grants.view"')
    require("auth", 'permission == "accounts.grants.modify"')

    for marker in (
        "isPublicAccountGrantResource",
        '"accounts.grants.view"',
        '"accounts.grants.modify"',
        'requestToAuthorize.backendId = "*"',
        "isPublicAccountGrantMutation",
    ):
        require("gate", marker)

    for marker in (
        "PublicAccountGrantResource",
        "PublicAccountGrantLookupResult",
        "PublicAccountGrantMutationRequest",
        "registerAccountGrantLookup",
        "registerAccountGrantMutation",
    ):
        require("runtime_h", marker)

    for marker in (
        "publicAccountGrantPath",
        'const std::string suffix("/grants")',
        "parsePublicAccountGrantMutationBody",
        "publicAccountGrantResponse",
        "publicStrongEntityTagResourceRevision",
        "publicAccountGrantRevision",
        "Account grant mutation requires one strong If-Match entity tag.",
        "public-api.accounts-grants-administration",
    ):
        require("runtime_cpp", marker)

    for forbidden in (
        "security_actor_permission_grants",
        "SecurityPermissionGrantRepository",
        "sqlite3",
    ):
        forbid("runtime_cpp", forbidden)

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
        "unsupportedWildcard",
        "unsupportedPermission",
        "replayEnsureWithStaleRevision",
        "revisionConflict",
        "replayRevokeWithStaleRevision",
        "grant_ensured",
        "grant_revoked",
    ):
        require("service_test", marker)

    for marker in (
        "/api/v1/accounts/account-a/grants",
        "publicStrongEntityTag",
        "request-missing-precondition",
        "request-stale-change",
        "response.statusCode == 412",
        "request-revoke",
    ):
        require("api_test", marker)

    for marker in (
        "accounts.grants.view",
        "accounts.grants.modify",
        "role.admin",
        "backend_scope_denied",
        "includeCsrf",
    ):
        require("gate_test", marker)

    for marker in (
        "# MU.7A — Bounded Backend Access Grant Administration",
        "IMPLEMENTED CANDIDATE",
        "channels.view@<concrete-backend>",
        "accounts.grants.view@*",
        "accounts.grants.modify@*",
        "security_actor_permission_grants",
        "local and real yaVDR acceptance pending",
        "role.admin@*",
        "MU.8",
        "MU.9",
    ):
        require("candidate", marker)

    for name in ("current", "status", "workstream", "roadmap", "phase_map"):
        require(name, "MU.7")
        require(name, "MU.7A")
    require("current", "PR #411 MERGED")
    require("status", "PR #411 MERGED")
    require("workstream", "PR #411 MERGED")

    for marker in (
        "test-security-human-account-grant-administration",
        "test-security-public-account-grants",
        "test-security-public-account-grant-gate",
        "python3 tools/check_mu7a_account_grant_administration.py",
    ):
        require("make", marker)

    occurrences = TEXT["make"].count(
        "python3 tools/check_mu7a_account_grant_administration.py"
    )
    if occurrences != 1:
        raise AssertionError(
            f"MU.7A guard must be wired exactly once, found {occurrences}"
        )

    print("MU.7A bounded Account grant administration contracts passed")
    print("MU6=DONE")
    print("MU6D=COMPLETED_REAL_YAVDR_PASS_PR411_MERGED")
    print("MU7=IN_PROGRESS")
    print("MU7A=IMPLEMENTED_CANDIDATE_ACCEPTANCE_PENDING")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.7A grant administration check failed:", file=sys.stderr)
        print(f"- {error}", file=sys.stderr)
        raise SystemExit(1)
