#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "account_h": ROOT / "core/security/include/HumanAccountRepository.h",
    "account_cpp": ROOT / "core/security/src/HumanAccountRepository.cpp",
    "admin_h": ROOT / "core/security/include/HumanAccountAdministrationService.h",
    "admin_cpp": ROOT / "core/security/src/HumanAccountAdministrationService.cpp",
    "browser_auth": ROOT / "core/security/include/BrowserSessionAuthenticator.h",
    "browser_repo_h": ROOT / "core/security/include/BrowserSessionCredentialRepository.h",
    "browser_repo_cpp": ROOT / "core/security/src/BrowserSessionCredentialRepository.cpp",
    "browser_lifecycle_h": ROOT / "core/security/include/BrowserSessionLifecycleService.h",
    "browser_lifecycle_cpp": ROOT / "core/security/src/BrowserSessionLifecycleService.cpp",
    "identity_h": ROOT / "core/security/include/SecurityIdentityRepository.h",
    "identity_cpp": ROOT / "core/security/src/SecurityIdentityRepository.cpp",
    "server": ROOT / "core/http/src/TestHttpServer.cpp",
    "service_test": ROOT / "core/security/tests/test_human_account_administration_service.cpp",
    "auth_test": ROOT / "core/security/tests/test_browser_session_authenticator.cpp",
    "current": ROOT / "docs/CURRENT.md",
    "workstream": ROOT / "docs/development/post-phase69-multiuser-workstream.md",
    "audit": ROOT / "docs/development/post-phase69-mu6-account-lifecycle-foundation.md",
    "roadmap": ROOT / "docs/planning/roadmap.md",
}

texts = {name: path.read_text(encoding="utf-8") for name, path in FILES.items()}


def require(name, marker):
    if marker not in texts[name]:
        raise AssertionError(f"{name} misses required marker: {marker}")


def forbid(name, marker):
    if marker in texts[name]:
        raise AssertionError(f"{name} contains forbidden marker: {marker}")


def main():
    for marker in (
        "std::uint64_t revision = 0",
        "revisionConflict",
        "updateDisplayNameInActiveTransaction",
        "setActiveInActiveTransaction",
    ):
        require("account_h", marker)

    for marker in (
        "revision INTEGER NOT NULL DEFAULT 1",
        "ALTER TABLE security_human_accounts",
        "revision = revision + 1",
        "WHERE account_id = ? AND revision = ?",
        "mutationMissStatus",
    ):
        require("account_cpp", marker)

    for marker in (
        "HumanAccountAdministrationStatus",
        "finalAdministrator",
        "modifyDisplayName",
        "setActive",
    ):
        require("admin_h", marker)

    for marker in (
        "countUsableAdministratorsExcludingActor",
        "final_usable_administrator",
        "revokeAllForActorInActiveTransaction",
        "accounts.modify",
        "accounts.activate",
        "accounts.deactivate",
        "account_revision_conflict",
    ):
        require("admin_cpp", marker)

    for marker in (
        "countUsableAdministratorsExcludingActor",
        "grant_record.permission = 'role.admin'",
        "credential.credential_type = 'human-password'",
        "security_basic_credential_verifiers",
    ):
        require("account_cpp", marker)

    forbid("admin_cpp", "#include <sqlite3.h>")
    forbid("admin_cpp", "sqlite3_")

    require("identity_h", "updateActorDisplayNameInActiveTransaction")
    require("identity_cpp", '"security_actors"')
    require("identity_cpp", '"display_name"')

    require("browser_repo_h", "findActiveByActorId")
    require("browser_repo_cpp", "findActiveByActorId")
    require("browser_repo_cpp", "AND active <> 0 AND revoked_at = ''")

    require("browser_lifecycle_h", "revokeAllForActorInActiveTransaction")
    require("browser_lifecycle_cpp", "revokeAllForActorInActiveTransaction")
    require("browser_lifecycle_cpp", "revokeInActiveTransaction")

    for marker in (
        "HumanAccountRepository",
        "findByActorId(record->actorId)",
        "HumanAccountRepositoryStatus::notFound",
        "AuthenticationState::Revoked",
    ):
        require("browser_auth", marker)

    # Production HTTP composition must actually pass the Human Account authority.
    if texts["server"].count("humanAccountRepository_.get()") < 2:
        raise AssertionError(
            "server must pass HumanAccountRepository to both browser authenticators"
        )

    for marker in (
        "HumanAccountAdministrationStatus::revisionConflict",
        "HumanAccountAdministrationStatus::finalAdministrator",
        "revokedBrowserSessions == 1U",
        "browserAfterReactivate",
        "account_deactivated",
    ):
        require("service_test", marker)

    for marker in (
        "accountAwareAuthenticator",
        "AuthenticationState::Revoked",
        "account-phase62-admin",
    ):
        require("auth_test", marker)

    for name in ("current", "workstream", "roadmap"):
        require(name, "MU.6")
        forbid(name, "MU.6 Human Account lifecycle administration not started")

    require("current", "MU.6A - Account lifecycle authority foundation [COMPLETED]")
    require("current", "MU.6B - Public Account item + revision/ETag [NOT STARTED]")
    require("workstream", "MU.6A Account lifecycle authority foundation            [DONE]")
    require("workstream", "MU.6B Public Account item + revision/ETag               [NEXT - NOT STARTED]")

    for marker in (
        "# MU.6A Human Account Lifecycle Authority Foundation",
        "Persisted Account revision",
        "Final usable administrator protection",
        "Browser-session defense in depth",
        "MU.6B Public Account item + revision/ETag",
        "Phase 70 remains not started",
    ):
        require("audit", marker)

    print("MU.6A Human Account lifecycle authority contracts passed")
    print("MU6=IN_PROGRESS")
    print("MU6A=COMPLETED_CANDIDATE")
    print("MU6B=NEXT_NOT_STARTED")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except AssertionError as error:
        print("MU.6A lifecycle architecture check failed:")
        print("- " + str(error))
        sys.exit(1)
