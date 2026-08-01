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
        "core/security/include/SecurityPermissionGrantRepository.h",
        "core/security/include/BrowserSessionAuthenticator.h",
        "core/security/include/BrowserSessionIssuanceService.h",
        "core/security/include/BrowserSessionLifecycleService.h",
        "core/security/include/BrowserSessionHttpGate.h",
        "core/security/include/SecurityIdentityRepository.h",
        "core/security/include/SecurityIdentityProvisioningRepository.h",
        "core/security/include/PersistentIdentityResolver.h",
        "core/security/include/AccountabilityEvent.h",
        "core/security/include/AccountabilityEventRepository.h",
        "core/security/include/SecurityHttpGate.h",
        "core/security/src/AccountabilityEventRepository.cpp",
        "core/security/src/BrowserSessionCredentialRepository.cpp",
        "core/security/src/SecurityPermissionGrantRepository.cpp",
        "core/security/src/BrowserSessionHttpGate.cpp",
        "core/security/src/BrowserSessionIssuanceService.cpp",
        "core/security/src/BrowserSessionLifecycleService.cpp",
        "core/security/src/CredentialVerifierRepository.cpp",
        "core/security/src/SecurityIdentityIssuanceRepository.cpp",
        "core/security/src/SecurityIdentityProvisioningRepository.cpp",
        "core/security/src/SecurityIdentityRepository.cpp",
        "core/http/include/BrowserSessionHttpService.h",
        "core/http/src/BrowserSessionHttpService.cpp",
        "core/security/tests/test_authorization_service.cpp",
        "core/security/tests/test_security_configuration.cpp",
        "core/security/tests/test_security_identity_repository.cpp",
        "core/security/tests/test_security_permission_grant_repository.cpp",
        "core/security/tests/test_managed_basic_authenticator.cpp",
        "core/security/tests/test_browser_session_authenticator.cpp",
        "core/security/tests/test_browser_session_issuance_service.cpp",
        "core/security/tests/test_browser_session_http_gate.cpp",
        "core/http/tests/test_browser_session_http_service.cpp",
        "core/security/tests/test_accountability_event_repository.cpp",
        "core/security/tests/test_security_http_gate.cpp",
        "web/frontend/platform/deferred-runtime-loader.js",
        "web/frontend/tests/test_channel_move_security_runtime.js",
        "web/frontend/tests/test_recording_execution_security_runtime.js",
        "docs/planning/phase-62-security-identity-gap-matrix.md",
        "docs/development/phase-62-slice-2i-recording-execution-security-migration.md",
        "docs/development/phase-62-slice-2j-searchtimer-create-security-migration.md",
        "docs/development/phase-62-security-identity-foundation-slice-1.md",
        "docs/development/phase-62-security-identity-foundation-slice-2.md",
        "docs/architecture/security-identity-foundation.md",
    ]

    for relative in required_files:
        if not (ROOT / relative).is_file():
            raise AssertionError(f"missing Phase 62 contract file: {relative}")

    require("core/http/src/TestHttpServer.cpp", "securityHttpGate_->evaluate(request)")
    require("core/http/src/TestHttpServer.cpp", "browserSessionHttpGate_->handles(request)")
    require("core/http/src/TestHttpServer.cpp", "browserSessionHttpService_->login(browserGate.context)")
    require("core/http/src/TestHttpServer.cpp", "browserSessionHttpService_->logout(browserGate.context)")
    require("core/http/src/TestHttpServer.cpp", "ensureCompatibilityIdentity")
    require("core/http/src/TestHttpServer.cpp", "securityIdentityProvisioningRepository_->ensureIdentity")
    require("core/http/src/TestHttpServer.cpp", "credentialVerifierRepository_->ensureVerifier")
    require("core/http/src/TestHttpServer.cpp", "persistentIdentityResolver_.get()")
    require("core/http/src/TestHttpServer.cpp", "managedBasicAuthenticator_.get()")
    require("core/security/include/SecurityHttpGate.h", '"/api/vdr/remote/actions"')
    require("core/security/include/AuthorizationService.h", '"channels.move"')
    require("core/security/include/SecurityHttpGate.h", '"/api/vdr/channels/move"')
    require("core/security/include/SecurityHttpGate.h", '"/api/vdr/channels/actions/move"')
    require("core/security/include/SecurityHttpGate.h", '"channels.move"')
    require(
        "core/security/include/SecurityHttpGate.h",
        '"/api/recordings/actions/execute"',
    )
    require(
        "core/security/include/SecurityHttpGate.h",
        '"/api/vdr/recordings/actions/execute"',
    )
    require("core/security/include/SecurityHttpGate.h", '"recordings.rename"')
    require("core/security/include/SecurityHttpGate.h", '"recordings.move"')
    require("core/security/include/SecurityHttpGate.h", '"recordings.delete"')
    require(
        "core/security/include/SecurityHttpGate.h",
        '"invalid_recording_action"',
    )
    require(
        "core/security/include/AuthorizationService.h",
        '"recordings.rename"',
    )
    require(
        "core/security/include/AuthorizationService.h",
        '"recordings.move"',
    )
    require(
        "core/security/include/AuthorizationService.h",
        '"recordings.delete"',
    )
    require("core/security/include/SecurityHttpGate.h", '"remote.control"')
    require("core/security/include/SecurityHttpGate.h", "usesLegacyCompatibilityCredential")
    require("core/security/include/SecurityHttpGate.h", "browserSessionAuthenticator_->verifyCsrf(request.headers)")
    require("core/security/include/SecurityHttpGate.h", '"csrf_validation_failed"')
    require("core/security/include/SecurityHttpGate.h", "isPost && gate.browserAuthenticated && !isRemoteAction")
    require("core/security/include/SecurityHttpGate.h", '"unmapped.browser.mutation"')
    require(
        "web/frontend/platform/deferred-runtime-loader.js",
        "__vdrSuiteChannelMoveMutationCsrfWrapped",
    )
    require(
        "web/frontend/platform/deferred-runtime-loader.js",
        "'/api/vdr/channels/move'",
    )
    require(
        "web/frontend/platform/deferred-runtime-loader.js",
        "'/api/vdr/channels/actions/move'",
    )
    require(
        "web/frontend/tests/test_channel_move_security_runtime.js",
        "caller-must-not-override",
    )
    require(
        "web/frontend/platform/deferred-runtime-loader.js",
        "__vdrSuiteRecordingExecutionMutationCsrfWrapped",
    )
    require(
        "web/frontend/platform/deferred-runtime-loader.js",
        "'/api/recordings/actions/execute'",
    )
    require(
        "web/frontend/platform/deferred-runtime-loader.js",
        "'/api/vdr/recordings/actions/execute'",
    )
    require(
        "web/frontend/tests/test_recording_execution_security_runtime.js",
        "caller-must-not-override",
    )
    require(
        "core/security/include/SecurityHttpGate.h",
        '"/api/searchtimers"',
    )
    require(
        "core/security/include/SecurityHttpGate.h",
        '"/api/vdr/searchtimers"',
    )
    require(
        "core/security/include/SecurityHttpGate.h",
        '"searchtimers.create"',
    )
    require(
        "core/security/include/AuthorizationService.h",
        '"searchtimers.create"',
    )
    require(
        "web/frontend/platform/deferred-runtime-loader.js",
        "__vdrSuiteSearchTimerCreateMutationCsrfWrapped",
    )
    require(
        "web/frontend/platform/deferred-runtime-loader.js",
        "'/api/searchtimers'",
    )
    require(
        "web/frontend/platform/deferred-runtime-loader.js",
        "'/api/vdr/searchtimers'",
    )
    require(
        "web/frontend/tests/test_searchtimer_workflows_runtime.js",
        "PHASE62_SLICE2J_SEARCHTIMER_CREATE_CSRF_TESTS",
    )
    require(
        "web/frontend/tests/test_searchtimer_workflows_runtime.js",
        "caller-must-not-override",
    )
    require(
        "docs/development/index.md",
        "phase-62-slice-2i-recording-execution-security-migration.md",
    )
    require(
        "docs/development/phase-62-slice-2i-recording-execution-security-migration.md",
        "recordings.rename",
    )
    require(
        "docs/development/phase-62-slice-2i-recording-execution-security-migration.md",
        "recordings.move",
    )
    require(
        "docs/development/phase-62-slice-2i-recording-execution-security-migration.md",
        "recordings.delete",
    )
    require(
        "docs/development/index.md",
        "phase-62-slice-2j-searchtimer-create-security-migration.md",
    )
    require(
        "docs/development/phase-62-slice-2j-searchtimer-create-security-migration.md",
        "searchtimers.create",
    )
    require(
        "docs/development/phase-62-slice-2j-searchtimer-create-security-migration.md",
        "POST /api/searchtimers",
    )
    require(
        "docs/development/phase-62-slice-2j-searchtimer-create-security-migration.md",
        "POST /api/vdr/searchtimers",
    )
    require(
        "docs/development/phase-62-slice-2j-searchtimer-create-security-migration.md",
        "Runtime acceptance remains pending",
    )
    require(
        "docs/development/phase-62-slice-2j-searchtimer-create-security-migration.md",
        "Runtime acceptance must not create a real SearchTimer",
    )
    require("core/security/tests/test_security_http_gate.cpp", '"csrf_validation_failed"')
    require("core/security/tests/test_security_http_gate.cpp", '"security_policy_not_migrated"')
    require("core/security/tests/test_security_http_gate.cpp", '"permission_denied"')
    require("core/security/tests/test_security_http_gate.cpp", '"backend_scope_denied"')
    require("core/security/src/BrowserSessionHttpGate.cpp", '"/api/security/browser-sessions"')
    require("core/security/src/BrowserSessionHttpGate.cpp", '"/api/security/browser-sessions/logout"')
    require("core/security/src/BrowserSessionHttpGate.cpp", "authenticateBasic(request)")
    require("core/security/src/BrowserSessionHttpGate.cpp", "authenticateBrowser(request)")
    require("core/security/src/BrowserSessionHttpGate.cpp", "browserAuthenticator_->verifyCsrf")
    require("core/security/src/BrowserSessionHttpGate.cpp", '"session.issue.self"')
    require("core/security/src/BrowserSessionHttpGate.cpp", '"session.revoke.self"')
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
    require(
        "core/security/src/SecurityPermissionGrantRepository.cpp",
        "security_actor_permission_grants",
    )
    require(
        "core/security/include/BrowserSessionAuthenticator.h",
        "findActiveGrantsForActor",
    )
    require(
        "core/security/include/SecurityHttpGate.h",
        "permission_grants_unavailable",
    )
    require("core/http/src/TestHttpServer.cpp", "securityPermissionGrantRepository_")
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
    require("core/security/src/BrowserSessionLifecycleService.cpp", 'database_.execute("BEGIN IMMEDIATE;")')
    require("core/security/src/BrowserSessionLifecycleService.cpp", "revokeBySessionId")
    require("core/http/src/BrowserSessionHttpService.cpp", 'response.headers["Set-Cookie"]')
    require("core/http/src/BrowserSessionHttpService.cpp", "HttpOnly; Secure; SameSite=Strict")
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
    require("core/http/src/TestHttpServer.cpp", "BrowserSessionAuthenticator")
    forbid("core/http/src/TestHttpServer.cpp", "vdr_suite_session=")
    forbid("core/security/include/SecurityHttpGate.h", "BrowserSessionIssuanceService")
    require("core/security/include/SecurityHttpGate.h", "BrowserSessionAuthenticator")
    require(
        "core/security/include/SecurityHttpGate.h",
        "browserSessionAuthenticator_->hasSessionCookie(",
    )
    require(
        "core/security/include/SecurityHttpGate.h",
        "browserSessionAuthenticator_->authenticate(",
    )
    require(
        "core/security/include/SecurityHttpGate.h",
        '"http.browser.mutation"',
    )
    forbid("core/security/include/SecurityHttpGate.h", "vdr_suite_session")
    forbid("core/security/src/SecurityIdentityRepository.cpp", "Authorization: Basic")
    forbid("core/security/src/SecurityIdentityRepository.cpp", "YWRtaW46")
    forbid("core/security/include/SecurityConfiguration.h", 'VDR_SUITE_MANAGED_BASIC_PASSWORD"')
    forbid("core/security/src/CredentialVerifierRepository.cpp", "decoded_password")
    forbid("core/security/src/BrowserSessionCredentialRepository.cpp", "session_secret TEXT")
    forbid("core/security/src/BrowserSessionCredentialRepository.cpp", "csrf_secret TEXT")
    forbid("core/security/src/BrowserSessionCredentialRepository.cpp", "cookie_value")

    repository_headers = [
        "core/security/include/AccountabilityEventRepository.h",
        "core/security/include/BrowserSessionCredentialRepository.h",
        "core/security/include/SecurityPermissionGrantRepository.h",
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
