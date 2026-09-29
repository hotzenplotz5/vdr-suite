#!/usr/bin/env python3
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "runtime_h": ROOT / "api/rest/include/PublicApiRuntime.h",
    "runtime_cpp": ROOT / "api/rest/src/PublicApiRuntime.cpp",
    "daemon_h": ROOT / "core/daemon/include/DaemonRuntime.h",
    "daemon_cpp": ROOT / "core/daemon/src/DaemonRuntimeInitialization.cpp",
    "gate": ROOT / "core/security/include/SecurityHttpGate.h",
    "auth": ROOT / "core/security/include/AuthorizationService.h",
    "account_repo": ROOT / "core/security/src/HumanAccountRepository.cpp",
    "account_service": ROOT / "core/security/src/HumanAccountReadService.cpp",
    "server_test": ROOT / "api/rest/tests/test_public_account_collection.cpp",
    "security_test": ROOT / "core/security/tests/test_public_account_collection_security.cpp",
    "client": ROOT / "clients/reference-js/public-v1-client.js",
    "client_test": ROOT / "clients/reference-js/tests/test_public_v1_account_client.js",
    "inventory": ROOT / "tools/check_phase69_public_api_inventory.py",
    "matrix": ROOT / "docs/development/phase-69f-client-contract-matrix.json",
    "matrix_guard": ROOT / "tools/check_phase69f_client_contract_matrix.py",
    "adr61": ROOT / "docs/adr/ADR-0061-actor-permissions-federation-client-access.md",
    "adr65": ROOT / "docs/adr/ADR-0065-human-account-profile-device-identity-boundary.md",
    "doc": ROOT / "docs/development/post-phase69-p2-public-account-collection.md",
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
        "PublicAccountCollectionItem",
        "PublicAccountCollectionRequest",
        "PublicAccountCollectionResult",
        "AccountCollectionLookup",
        "registerAccountCollectionLookup",
    ):
        require("runtime_h", marker)

    for marker in (
        '"/api/v1/accounts"',
        '"accounts/1|"',
        '"ac1_"',
        "publicAccountCollectionResponse",
        "accountCollectionLookupConfigured()",
        "public-api.accounts-read",
    ):
        require("runtime_cpp", marker)

    for forbidden in ("sqlite3", "HumanAccountRepository"):
        forbid("runtime_h", forbidden)
        forbid("runtime_cpp", forbidden)

    require("daemon_h", "HumanAccountReadService")
    require("daemon_cpp", "humanAccountReadService_->list()")
    require("daemon_cpp", "registerAccountCollectionLookup")
    require("daemon_cpp", "HumanAccountRepository")

    for marker in (
        'path == "/api/v1/accounts"',
        'accountReadRequest.permission = "accounts.view"',
        'accountReadRequest.backendId = "*"',
        'accountReadRequest.action = "accounts.view"',
    ):
        require("gate", marker)

    require("auth", 'permission == "accounts.view"')
    require("account_repo", "security_human_accounts")
    require("account_service", "listAll")

    for name in ("runtime_h", "runtime_cpp"):
        for forbidden in (
            "passwordHash",
            "credentialId",
            "sessionSecret",
            "csrfSecret",
        ):
            forbid(name, forbidden)

    for marker in (
        "accountId",
        "actorId",
        "displayName",
        "active",
        "hasMore",
        "ac1_",
        "limit=101",
        "sort=displayName",
    ):
        require("server_test", marker)

    for marker in (
        'constexpr const char* Permission = "accounts.view"',
        '"role.admin"',
        '"backend_scope_denied"',
        '"permission_denied"',
        "statusCode == 401",
    ):
        require("security_test", marker)

    require("client", "getAccounts(options)")
    require("client", "'/api/v1/accounts'")
    require("client_test", "test_public_v1_account_client passed")
    require("inventory", '"/api/v1/accounts"')

    matrix = json.loads(read("matrix"))
    resources = {
        (item.get("method"), item.get("template"))
        for item in matrix.get("publicV1Resources", [])
    }
    if len(resources) != 9:
        raise AssertionError(
            f"expected 9 stable Public-v1 contracts, got {len(resources)}"
        )
    if ("GET", "/api/v1/accounts") not in resources:
        raise AssertionError("Account collection missing from Public-v1 matrix")

    references = matrix.get("publicClientReferences", [])
    if len(references) != 7:
        raise AssertionError(
            f"expected 7 Public-v1 reference slices, got {len(references)}"
        )
    account_reference = next(
        (
            item
            for item in references
            if item.get("id") == "reference-js-account-collection"
        ),
        None,
    )
    if account_reference is None:
        raise AssertionError("Account reference client is not registered")
    if account_reference.get("resources") != ["GET /api/v1/accounts"]:
        raise AssertionError("Account reference resource drifted")

    require("matrix_guard", '("GET", "/api/v1/accounts")')
    require("matrix_guard", "expected exactly seven bounded public client reference slices")
    require("matrix_guard", "Stable public-v1 method/resource contracts: 9.")

    require("adr61", "accounts.view@*")
    require("adr65", "accounts.view@*")
    require("doc", "GET /api/v1/accounts")
    require("doc", "HTTP code does not query SQLite")
    require("make", "test-security-public-account-collection")

    for forbidden in (
        '"/api/v1/accounts/"',
        "createAccount",
        "updateAccount",
        "deleteAccount",
    ):
        forbid("runtime_cpp", forbidden)
        forbid("client", forbidden)

    print("P2 public Account collection contracts passed")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"P2 public Account collection check failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
