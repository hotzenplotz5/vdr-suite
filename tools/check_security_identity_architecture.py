#!/usr/bin/env python3

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def require(path: str, needle: str) -> None:
    text = (ROOT / path).read_text(encoding="utf-8")
    if needle not in text:
        raise AssertionError(f"{path}: missing required contract: {needle}")


def forbid(path: str, needle: str) -> None:
    text = (ROOT / path).read_text(encoding="utf-8")
    if needle in text:
        raise AssertionError(f"{path}: forbidden contract remains active: {needle}")


def main() -> int:
    required_files = [
        "core/security/include/SecurityIdentity.h",
        "core/security/include/AuthorizationService.h",
        "core/security/include/SecurityConfiguration.h",
        "core/security/include/LegacyBasicAuthenticator.h",
        "core/security/include/ManagedBasicAuthenticator.h",
        "core/security/include/CredentialVerifierRepository.h",
        "core/security/include/SecurityIdentityRepository.h",
        "core/security/include/SecurityIdentityProvisioningRepository.h",
        "core/security/include/PersistentIdentityResolver.h",
        "core/security/include/AccountabilityEvent.h",
        "core/security/include/AccountabilityEventRepository.h",
        "core/security/include/SecurityHttpGate.h",
        "core/security/tests/test_authorization_service.cpp",
        "core/security/tests/test_security_configuration.cpp",
        "core/security/tests/test_security_identity_repository.cpp",
        "core/security/tests/test_managed_basic_authenticator.cpp",
        "core/security/tests/test_accountability_event_repository.cpp",
        "core/security/tests/test_security_http_gate.cpp",
        "docs/planning/phase-62-security-identity-gap-matrix.md",
        "docs/development/phase-62-security-identity-foundation-slice-1.md",
        "docs/development/phase-62-security-identity-foundation-slice-2.md",
        "docs/architecture/security-identity-foundation.md",
    ]

    for relative in required_files:
        if not (ROOT / relative).is_file():
            raise AssertionError(f"missing Phase 62 contract file: {relative}")

    require(
        "core/http/src/TestHttpServer.cpp",
        "securityHttpGate_->evaluate(request)")
    require(
        "core/http/src/TestHttpServer.cpp",
        "ensureCompatibilityIdentity")
    require(
        "core/http/src/TestHttpServer.cpp",
        "securityIdentityProvisioningRepository_->ensureIdentity")
    require(
        "core/http/src/TestHttpServer.cpp",
        "credentialVerifierRepository_->ensureVerifier")
    require(
        "core/http/src/TestHttpServer.cpp",
        "persistentIdentityResolver_.get()")
    require(
        "core/http/src/TestHttpServer.cpp",
        "managedBasicAuthenticator_.get()")
    require(
        "core/security/include/SecurityHttpGate.h",
        '"/api/vdr/remote/actions"')
    require(
        "core/security/include/SecurityHttpGate.h",
        '"remote.control"')
    require(
        "core/security/include/SecurityHttpGate.h",
        "usesLegacyCompatibilityCredential")
    require(
        "core/security/include/SecurityIdentityRepository.h",
        "security_actors")
    require(
        "core/security/include/SecurityIdentityRepository.h",
        "security_devices")
    require(
        "core/security/include/SecurityIdentityRepository.h",
        "security_sessions")
    require(
        "core/security/include/SecurityIdentityRepository.h",
        "security_credentials")
    require(
        "core/security/include/SecurityIdentityRepository.h",
        "INSERT OR IGNORE")
    require(
        "core/security/include/SecurityIdentityProvisioningRepository.h",
        "INSERT OR IGNORE INTO security_actors")
    require(
        "core/security/include/CredentialVerifierRepository.h",
        "security_basic_credential_verifiers")
    require(
        "core/security/include/ManagedBasicAuthenticator.h",
        "crypt_r")
    require(
        "core/security/include/ManagedBasicAuthenticator.h",
        'passwordHash.rfind("$y$", 0)')
    require(
        "core/security/include/ManagedBasicAuthenticator.h",
        'passwordHash.rfind("$6$", 0)')
    require(
        "core/security/include/PersistentIdentityResolver.h",
        "findCredential")
    require(
        "core/security/include/AccountabilityEventRepository.h",
        "accountability_events_no_update")
    require(
        "core/security/include/AccountabilityEventRepository.h",
        "accountability_events_no_delete")
    require(
        "core/security/include/SecurityConfiguration.h",
        "LegacyBasicCompatibility")
    require(
        "core/security/include/SecurityConfiguration.h",
        'VDR_SUITE_SECURITY_MODE')
    require(
        "core/security/include/SecurityConfiguration.h",
        'VDR_SUITE_LEGACY_BASIC_CREDENTIAL_ID')
    require(
        "core/security/include/SecurityConfiguration.h",
        'VDR_SUITE_MANAGED_BASIC_USERNAME')
    require(
        "core/security/include/SecurityConfiguration.h",
        'VDR_SUITE_MANAGED_BASIC_PASSWORD_HASH')
    require(
        "core/security/include/SecurityHttpGate.h",
        "security_policy_not_migrated")
    require(
        "core/security/include/SecurityHttpGate.h",
        "credential_revoked")
    require(
        "docs/development/phase-62-security-identity-foundation-slice-2.md",
        "Real-VDR acceptance of the persistence/revocation foundation")
    forbid(
        "core/http/src/TestHttpServer.cpp",
        "isAuthorized(request)")
    forbid(
        "core/http/src/TestHttpServerAssets.inc",
        "expectedAuthorizationHeader")
    forbid(
        "core/http/src/TestHttpServerAssets.inc",
        "isAuthorized")
    forbid(
        "core/security/include/SecurityIdentityRepository.h",
        "Authorization: Basic")
    forbid(
        "core/security/include/SecurityIdentityRepository.h",
        "YWRtaW46")
    forbid(
        "core/security/include/SecurityConfiguration.h",
        "VDR_SUITE_MANAGED_BASIC_PASSWORD\"")
    forbid(
        "core/security/include/CredentialVerifierRepository.h",
        "decoded_password")

    print("security identity architecture contracts passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"security identity architecture check failed: {error}", file=sys.stderr)
        raise SystemExit(1)
