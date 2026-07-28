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
        "core/security/include/BrowserSessionCredentialRepository.h",
        "core/security/include/BrowserSessionAuthenticator.h",
        "core/security/include/BrowserSessionIssuanceService.h",
        "core/security/include/SecurityIdentityRepository.h",
        "core/security/include/SecurityIdentityProvisioningRepository.h",
        "core/security/include/PersistentIdentityResolver.h",
        "core/security/include/AccountabilityEvent.h",
        "core/security/include/AccountabilityEventRepository.h",
        "core/security/include/SecurityHttpGate.h",
        "core/security/src/AccountabilityEventRepository.cpp",
        "core/security/src/BrowserSessionCredentialRepository.cpp",
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "core/security/src/CredentialVerifierRepository.cpp",
        "core/security/src/SecurityIdentityIssuanceRepository.cpp",
        "core/security/src/SecurityIdentityProvisioningRepository.cpp",
        "core/security/src/SecurityIdentityRepository.cpp",
        "core/security/tests/test_authorization_service.cpp",
        "core/security/tests/test_security_configuration.cpp",
        "core/security/tests/test_security_identity_repository.cpp",
        "core/security/tests/test_managed_basic_authenticator.cpp",
        "core/security/tests/test_browser_session_authenticator.cpp",
        "core/security/tests/test_browser_session_issuance_service.cpp",
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

    require("core/http/src/TestHttpServer.cpp", "securityHttpGate_->evaluate(request)")
    require("core/http/src/TestHttpServer.cpp", "ensureCompatibilityIdentity")
    require("core/http/src/TestHttpServer.cpp", "securityIdentityProvisioningRepository_->ensureIdentity")
    require("core/http/src/TestHttpServer.cpp", "credentialVerifierRepository_->ensureVerifier")
    require("core/http/src/TestHttpServer.cpp", "persistentIdentityResolver_.get()")
    require("core/http/src/TestHttpServer.cpp", "managedBasicAuthenticator_.get()")
    require("core/security/include/SecurityHttpGate.h", '"/api/vdr/remote/actions"')
    require("core/security/include/SecurityHttpGate.h", '"remote.control"')
    require("core/security/include/SecurityHttpGate.h", "usesLegacyCompatibilityCredential")
    require("core/security/src/SecurityIdentityRepository.cpp", "security_actors")
    require("core/security/src/SecurityIdentityRepository.cpp", "security_devices")
    require("core/security/src/SecurityIdentityRepository.cpp", "security_sessions")
    require("core/security/src/SecurityIdentityRepository.cpp", "security_credentials")
    require("core/security/src/SecurityIdentityRepository.cpp", "INSERT OR IGNORE")
    require("core/security/src/SecurityIdentityProvisioningRepository.cpp", "INSERT OR IGNORE INTO security_actors")
    require("core/security/src/CredentialVerifierRepository.cpp", "security_basic_credential_verifiers")
    require("core/security/src/BrowserSessionCredentialRepository.cpp", "security_browser_session_credentials")
    require("core/security/src/BrowserSessionCredentialRepository.cpp", "session_secret_hash")
    require("core/security/src/BrowserSessionCredentialRepository.cpp", "csrf_secret_hash")
    require("core/security/src/BrowserSessionCredentialRepository.cpp", "issued_from_credential_id")
    require("core/security/include/ManagedBasicAuthenticator.h", "crypt_r")
    require("core/security/include/ManagedBasicAuthenticator.h", 'passwordHash.rfind("$y$", 0)')
    require("core/security/include/ManagedBasicAuthenticator.h", 'passwordHash.rfind("$6$", 0)')
    require("core/security/include/BrowserSessionAuthenticator.h", '"vdr_suite_session"')
    require("core/security/include/BrowserSessionAuthenticator.h", '"X-CSRF-Token"')
    require("core/security/include/BrowserSessionAuthenticator.h", "crypt_r")
    require("core/security/include/BrowserSessionAuthenticator.h", "verifyCsrf")
    require("core/security/src/BrowserSessionIssuanceService.cpp", "getrandom(")
    require("core/security/src/BrowserSessionIssuanceService.cpp", 'database_.execute("BEGIN IMMEDIATE;")')
    require("core/security/src/BrowserSessionIssuanceService.cpp", "clearSecrets()")
    require("core/security/src/SecurityIdentityIssuanceRepository.cpp", "INSERT INTO security_sessions")
    require("core/security/src/SecurityIdentityIssuanceRepository.cpp", "INSERT INTO security_credentials")
    require("core/security/include/PersistentIdentityResolver.h", "findCredential")
    require("core/security/src/AccountabilityEventRepository.cpp", "accountability_events_no_update")
    require("core/security/src/AccountabilityEventRepository.cpp", "accountability_events_no_delete")
    require("core/security/include/SecurityConfiguration.h", "LegacyBasicCompatibility")
    require("core/security/include/SecurityConfiguration.h", 'VDR_SUITE_SECURITY_MODE')
    require("core/security/include/SecurityConfiguration.h", 'VDR_SUITE_LEGACY_BASIC_CREDENTIAL_ID')
    require("core/security/include/SecurityConfiguration.h", 'VDR_SUITE_MANAGED_BASIC_USERNAME')
    require("core/security/include/SecurityConfiguration.h", 'VDR_SUITE_MANAGED_BASIC_PASSWORD_HASH')
    require("core/security/include/SecurityHttpGate.h", "security_policy_not_migrated")
    require("core/security/include/SecurityHttpGate.h", "credential_revoked")
    require("docs/development/phase-62-security-identity-foundation-slice-2.md", "Real-VDR acceptance of the persistence/revocation foundation")
    require("docs/development/phase-62-security-identity-foundation-slice-2.md", "Atomic browser-session issuance contract")
    require("docs/architecture/security-identity-foundation.md", "Browser-session issuance")

    forbid("core/http/src/TestHttpServer.cpp", "isAuthorized(request)")
    forbid("core/http/src/TestHttpServerAssets.inc", "expectedAuthorizationHeader")
    forbid("core/http/src/TestHttpServerAssets.inc", "isAuthorized")
    forbid("core/http/src/TestHttpServer.cpp", "BrowserSessionIssuanceService")
    forbid("core/security/include/SecurityHttpGate.h", "BrowserSessionIssuanceService")
    forbid("core/security/src/SecurityIdentityRepository.cpp", "Authorization: Basic")
    forbid("core/security/src/SecurityIdentityRepository.cpp", "YWRtaW46")
    forbid("core/security/include/SecurityConfiguration.h", "VDR_SUITE_MANAGED_BASIC_PASSWORD\"")
    forbid("core/security/src/CredentialVerifierRepository.cpp", "decoded_password")
    forbid("core/security/src/BrowserSessionCredentialRepository.cpp", "session_secret TEXT")
    forbid("core/security/src/BrowserSessionCredentialRepository.cpp", "csrf_secret TEXT")
    forbid("core/security/src/BrowserSessionCredentialRepository.cpp", "cookie_value")

    repository_headers = [
        "core/security/include/AccountabilityEventRepository.h",
        "core/security/include/BrowserSessionCredentialRepository.h",
        "core/security/include/CredentialVerifierRepository.h",
        "core/security/include/SecurityIdentityProvisioningRepository.h",
        "core/security/include/SecurityIdentityRepository.h",
    ]
    for header in repository_headers:
        forbid(header, "sqlite3.h")
        forbid(header, "sqlite3_")

    print("security identity architecture contracts passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"security identity architecture check failed: {error}", file=sys.stderr)
        raise SystemExit(1)
