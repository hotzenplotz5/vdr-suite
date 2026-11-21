#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "service_h": ROOT / "core/security/include/FirstAdminClaimService.h",
    "service_cpp": ROOT / "core/security/src/FirstAdminClaimService.cpp",
    "identity_h": ROOT / "core/security/include/SecurityIdentityProvisioningRepository.h",
    "identity_cpp": ROOT / "core/security/src/SecurityIdentityProvisioningRepository.cpp",
    "account_h": ROOT / "core/security/include/HumanAccountRepository.h",
    "account_cpp": ROOT / "core/security/src/HumanAccountRepository.cpp",
    "bootstrap": ROOT / "core/security/src/FirstAdminBootstrapRepository.cpp",
    "test": ROOT / "core/security/tests/test_first_admin_claim_service.cpp",
    "make": ROOT / "mk/security-sources.mk",
    "audit": ROOT / "docs/development/post-phase69-p2-first-admin-bootstrap-audit.md",
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
        "FirstAdminClaimRequest",
        "FirstAdminClaimResult",
        "FirstAdminClaimStatus",
        "EntropySource",
        "clearSecrets",
    ):
        require("service_h", marker)

    for marker in (
        'database_.execute("BEGIN IMMEDIATE;")',
        "bootstrapRepository_.claimState()",
        "bootstrapRepository_.findById",
        "verifySecret(",
        "constantTimeEqual",
        "crypt_r(",
        "crypt_gensalt_rn(",
        '"$y$"',
        "bootstrapRepository_.consumeInActiveTransaction",
        "ensureHumanCredentialInActiveTransaction",
        "ensureAccountInActiveTransaction",
        "credentialVerifierRepository_.ensureVerifier",
        '"human-password"',
        '"role.admin"',
        'permissionGrantRepository_.ensureGrant',
        'accountabilityRepository_.append(event)',
        '"security.first-admin.claim"',
        '"bootstrap_verified"',
        'transaction.commit()',
    ):
        require("service_cpp", marker)

    require(
        "identity_h",
        "ensureHumanCredentialInActiveTransaction",
    )
    require(
        "identity_cpp",
        "database_.transactionActive()",
    )
    require(
        "identity_cpp",
        "actorTypeName(ActorType::User)",
    )
    require(
        "account_h",
        "ensureAccountInActiveTransaction",
    )
    require(
        "account_cpp",
        "database_.transactionActive()",
    )

    for marker in (
        "FirstAdminClaimStatus::success",
        'verifier->passwordHash.rfind("$y$", 0) == 0',
        'grants.grants[0].permission == "role.admin"',
        '"security.first-admin.claim"',
        '"bootstrap-rollback"',
        '"ace_606162636465666768696a6b6c6d6e6f"',
        '"actor_404142434445464748494a4b4c4d4e4f"',
        '"credential_505152535455565758595a5b5c5d5e5f"',
        "rolledBackGrants.grants.empty()",
        'events[0].eventType == "test.duplicate"',
        "FirstAdminClaimStatus::bootstrapRejected",
        "FirstAdminClaimStatus::claimed",
    ):
        require("test", marker)

    for marker in (
        "core/security/src/FirstAdminClaimService.cpp",
        "test-security-first-admin-claim-service:",
        "python3 tools/check_p2_first_admin_claim.py",
    ):
        require("make", marker)

    for marker in (
        "atomic first-admin claim service",
        "yescrypt",
        "single SQLite transaction",
        "rollback",
        "Claim-only browser completion boundary",
        "Legacy Basic runtime implementation removal",
    ):
        require("audit", marker)

    forbid("service_cpp", "INSERT INTO security_actors")
    forbid("service_cpp", "INSERT INTO security_credentials")
    forbid("service_cpp", "INSERT INTO security_human_accounts")
    forbid("service_cpp", "INSERT INTO security_actor_permission_grants")
    forbid("service_cpp", "INSERT INTO accountability_events")
    forbid("service_cpp", "Http")
    forbid("service_cpp", "VDR_SUITE_SECURITY_MODE")
    forbid("service_h", "sqlite3")
    forbid("account_h", "sqlite3")
    forbid("identity_h", "sqlite3")
    forbid("test", "sqlite3")
    forbid("test", "SELECT ")
    forbid("test", "INSERT ")
    forbid("test", "UPDATE ")
    forbid("test", "DELETE ")
    forbid("test", "DROP TABLE")

    print("P2 atomic first-admin claim contracts passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"P2 atomic first-admin claim check failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
