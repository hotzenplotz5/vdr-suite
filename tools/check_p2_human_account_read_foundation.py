#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "repo_h": ROOT / "core/security/include/HumanAccountRepository.h",
    "repo_cpp": ROOT / "core/security/src/HumanAccountRepository.cpp",
    "service_h": ROOT / "core/security/include/HumanAccountReadService.h",
    "service_cpp": ROOT / "core/security/src/HumanAccountReadService.cpp",
    "test": ROOT / "core/security/tests/test_human_account_read_foundation.cpp",
    "adr": ROOT / "docs/adr/ADR-0065-human-account-profile-device-identity-boundary.md",
    "index": ROOT / "docs/adr/index.md",
    "doc": ROOT / "docs/development/post-phase69-p2-human-account-read-foundation.md",
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
        "security_human_accounts",
        "actor_id TEXT NOT NULL UNIQUE",
        "actor_type = 'user'",
        "security_human_accounts_require_user_insert",
        "security_human_accounts_preserve_user_actor",
        "ORDER BY account.account_id ASC",
    ):
        require("repo_cpp", marker)

    require("repo_h", "HumanAccountRecord")
    require("repo_h", "HumanAccountRepositoryStatus")
    require("service_h", "HumanAccountReadService")
    require("service_cpp", "findByAccountId")
    require("service_cpp", "listAll")

    for name in ("repo_h", "repo_cpp", "service_h", "service_cpp"):
        for forbidden in (
            "password",
            "passwordHash",
            "credentialId",
            "sessionSecret",
            "csrf",
        ):
            forbid(name, forbidden)

    require("test", "compatibilityOnly.accounts.empty()")
    require("test", "service-actor-1")
    require("test", "assert(!database.execute")
    require("test", "revokeActor")
    require("adr", "Accepted architecture.")
    require(
        "index",
        "ADR-0065-human-account-profile-device-identity-boundary.md",
    )
    require(
        "doc",
        "No public `/api/v1/accounts` route is added in this slice.",
    )
    require(
        "make",
        "test-security-human-account-read-foundation",
    )

    print("P2 human account read foundation contracts passed")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"P2 human account read foundation check failed: {error}", file=sys.stderr)
        raise SystemExit(1)
