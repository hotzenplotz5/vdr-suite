#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "runtime_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "runtime_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "authorization": ROOT / "core/security/include/AuthorizationService.h",
    "gate": ROOT / "core/security/include/SecurityHttpGate.h",
    "server_h": ROOT / "core/http/include/TestHttpServer.h",
    "server_cpp": ROOT / "core/http/src/TestHttpServer.cpp",
    "admin_service": ROOT / "core/security/src/HumanAccountAdministrationService.cpp",
    "api_test": ROOT / "api/rest/tests/test_public_account_collection.cpp",
    "security_test": ROOT / "core/security/tests/test_public_account_collection_security.cpp",
    "client": ROOT / "clients/reference-js/public-v1-client.js",
    "client_test": ROOT / "clients/reference-js/tests/test_public_v1_account_client.js",
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
    "candidate": ROOT / "docs/development/post-phase69-mu6c-public-account-lifecycle-mutation.md",
    "make": ROOT / "mk/security-sources.mk",
}


def read(name):
    path = FILES[name]
    if not path.exists():
        raise AssertionError(f"missing {path}")
    return path.read_text(encoding="utf-8")


def require(name, marker):
    if marker not in read(name):
        raise AssertionError(f"{FILES[name]} missing marker: {marker}")


def forbid(name, marker):
    if marker in read(name):
        raise AssertionError(f"{FILES[name]} contains forbidden marker: {marker}")


def main():
    for marker in (
        "PublicAccountMutationKind",
        "PublicAccountMutationStatus",
        "PublicAccountMutationRequest",
        "PublicAccountMutationResult",
        "AccountMutation",
        "registerAccountMutation",
        "resetAccountMutation",
        "accountMutationConfigured",
    ):
        require("runtime_h", marker)

    for marker in (
        "parsePublicAccountMutationBody",
        "publicAccountRevision",
        "Account mutation requires one strong If-Match entity tag.",
        "publicStrongEntityTagResourceRevision",
        "PublicAccountMutationStatus::revisionConflict",
        "PublicAccountMutationStatus::finalAdministrator",
        "public-api.accounts-lifecycle-mutation",
        "GET, POST",
    ):
        require("runtime_cpp", marker)

    for marker in (
        'permission == "accounts.modify"',
        'permission == "accounts.activate"',
        'permission == "accounts.deactivate"',
    ):
        require("authorization", marker)

    for marker in (
        "isPublicAccountMutation",
        "publicAccountMutationAuthorization",
        'requestToAuthorize.backendId = "*"',
        '"accounts.modify"',
        '"accounts.activate"',
        '"accounts.deactivate"',
        "gate.protectedMutation = isProtectedMutation",
    ):
        require("gate", marker)

    for marker in (
        "HumanAccountAdministrationRepository",
        "HumanAccountAdministrationService",
        "~TestHttpServer() override",
    ):
        require("server_h", marker)

    for marker in (
        "registerAccountMutation",
        "humanAccountAdministrationService_",
        "modifyDisplayName",
        "setActive",
        "resetAccountMutation",
        "findActor",
    ):
        require("server_cpp", marker)

    for marker in (
        "countUsableAdministratorsExcludingActor",
        "revokeAllForActorInActiveTransaction",
        "accounts.modify",
        "accounts.activate",
        "accounts.deactivate",
        "final_usable_administrator",
    ):
        require("admin_service", marker)

    for marker in (
        "missingIfMatch.statusCode == 428",
        "stale.statusCode == 412",
        "finalAdministrator.statusCode == 409",
        "unsupportedMediaType.statusCode == 415",
        'renamed.headers.count("ETag") == 1U',
    ):
        require("api_test", marker)

    for marker in (
        '"accounts.modify"',
        '"accounts.activate"',
        '"accounts.deactivate"',
        '"role.admin"',
        '"role.read-only"',
        "includeCsrf",
        "backend_scope_denied",
        "role_read_only",
    ):
        require("security_test", marker)

    for marker in (
        "requestAccountMutation",
        "updateAccountDisplayName",
        "activateAccount",
        "deactivateAccount",
        "headers['If-Match']",
    ):
        require("client", marker)

    for marker in (
        "client.updateAccountDisplayName",
        "client.deactivateAccount",
        "client.activateAccount",
        "revision_conflict",
        "X-CSRF-Token",
    ):
        require("client_test", marker)

    for name in (
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
    ):
        require(name, "MU.6C")
        require(name, "LOCAL ACCEPTANCE")
        require(name, "MU.6D")

    for name in (
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
    ):
        forbid(name, "MU.6C public lifecycle mutation is next")

    for marker in (
        "# MU.6C Public Account Lifecycle Mutation",
        "COMPLETED — real yaVDR acceptance passed; PR #410 merged",
        "POST /api/v1/accounts/{accountId}",
        "accounts.modify",
        "accounts.activate",
        "accounts.deactivate",
        "role.admin@default",
        "does not require a bootstrap or code patch",
    ):
        require("candidate", marker)

    for marker in (
        "python3 tools/check_mu6c_public_account_lifecycle_mutation.py",
    ):
        require("make", marker)

    for name in ("runtime_cpp", "client"):
        for forbidden in (
            "accounts.grants.modify",
            "accounts.credentials.revoke",
            "accounts.sessions.revoke",
        ):
            forbid(name, forbidden)

    print("MU.6C Public Account lifecycle mutation contracts passed")
    print("MU6=IN_PROGRESS")
    print("MU6A=DONE")
    print("MU6B=DONE")
    print("MU6C=COMPLETED")
    print("MU6D_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU6C_GUARD")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.6C lifecycle mutation check failed:", file=sys.stderr)
        print(f"- {error}", file=sys.stderr)
        raise SystemExit(1)
