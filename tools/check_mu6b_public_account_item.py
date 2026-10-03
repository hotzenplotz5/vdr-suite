#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "runtime_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "runtime_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "daemon": ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp",
    "gate": ROOT / "core/security/include/SecurityHttpGate.h",
    "account_service": ROOT / "core/security/src/HumanAccountReadService.cpp",
    "server_test": ROOT / "api/rest/tests/test_public_account_collection.cpp",
    "security_test": ROOT / "core/security/tests/test_public_account_collection_security.cpp",
    "client": ROOT / "clients/reference-js/public-v1-client.js",
    "client_test": ROOT / "clients/reference-js/tests/test_public_v1_account_client.js",
    "current": ROOT / "docs/CURRENT.md",
    "current_status": ROOT / "docs/development/current-status.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
    "roadmap": ROOT / "docs/planning/roadmap.md",
    "phase_map": ROOT / "docs/planning/phase-map.md",
    "closeout": ROOT / "docs/development/post-phase69-mu6b-public-account-item.md",
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
        "PublicAccountLookupStatus",
        "PublicAccountResource",
        "PublicAccountLookupResult",
        "AccountLookup",
        "registerAccountLookup",
        "lookupAccount",
    ):
        require("runtime_h", marker)

    for marker in (
        '"/api/v1/accounts/"',
        "publicAccountPath",
        "publicAccountResponse",
        "publicStrongEntityTag",
        "publicEvaluateIfNoneMatch",
        "PublicAccountLookupStatus::notFound",
        "PublicAccountLookupStatus::unavailable",
    ):
        require("runtime_cpp", marker)

    for forbidden in (
        "passwordHash",
        "sessionSecret",
        "csrfSecret",
    ):
        forbid("runtime_h", forbidden)
        forbid("runtime_cpp", forbidden)

    for marker in (
        "registerAccountLookup",
        "humanAccountReadService_->find(accountId)",
        '"account:"',
        "std::to_string(found.account.revision)",
    ):
        require("daemon", marker)

    require("account_service", "repository_.findByAccountId(accountId)")

    for marker in (
        '"/api/v1/accounts/"',
        "isPublicAccountResource",
        'accountReadRequest.permission = "accounts.view"',
        'accountReadRequest.backendId = "*"',
        'accountReadRequest.action = "accounts.view"',
    ):
        require("gate", marker)

    for marker in (
        '"/api/v1/accounts/account-a"',
        'item.headers.count("ETag") == 1U',
        "notModified.statusCode == 304",
        "missingItem.statusCode == 404",
        "invalidItem.statusCode == 400",
        "unavailableItem.statusCode == 503",
        'item.body.find("resourceRevision") == std::string::npos',
        'first.headers.find("ETag") == first.headers.end()',
    ):
        require("server_test", marker)

    for marker in (
        'browserGet(fixture, "/api/v1/accounts/account-a")',
        'constexpr const char* Permission = "accounts.view"',
        '"backend_scope_denied"',
        "statusCode == 401",
    ):
        require("security_test", marker)

    for marker in (
        "function accountItemPath(options)",
        "getAccount(options)",
        "requestRevisioned(",
        "'/api/v1/accounts/' + options.accountId",
    ):
        require("client", marker)

    for marker in (
        "client.getAccount",
        "If-None-Match",
        "notModified.status, 304",
        "test_public_v1_account_client passed",
    ):
        require("client_test", marker)

    for name in ("current", "current_status", "workstream", "roadmap", "phase_map"):
        require(name, "MU.6B")
        require(name, "MU.6C")

    # MU.6B is completed and must remain discoverable as durable evidence,
    # but it must not own CURRENT's volatile active-slice status after successors land.
    require("current", "MU.6B Public Account Item + Revision/ETag")
    require("workstream", "MU.6B Public Account item + revision/ETag")
    require("workstream", "MU.6C Public display-name / activate / deactivate")

    for marker in (
        "# MU.6B Public Account Item + Revision/ETag",
        "GET /api/v1/accounts/{accountId}",
        "accounts.view@*",
        "If-None-Match",
        "strong ETag",
        "Collection remains unchanged",
        "MU.6C",
        "Phase 70 remains not started",
    ):
        require("closeout", marker)

    require("make", "python3 tools/check_mu6b_public_account_item.py")

    for forbidden in (
        "deleteAccount",
        "accounts.grants.modify",
        "accounts.credentials.revoke",
        "accounts.sessions.revoke",
    ):
        forbid("runtime_cpp", forbidden)
        forbid("client", forbidden)

    print("MU.6B Public Account item contracts passed")
    print("MU6=IN_PROGRESS")
    print("MU6A=DONE")
    print("MU6B=COMPLETED")
    print("MU6C_STATUS=SUCCESSOR_STATUS_NOT_OWNED_BY_MU6B_GUARD")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print("MU.6B Public Account item check failed:", file=sys.stderr)
        print(f"- {error}", file=sys.stderr)
        raise SystemExit(1)
