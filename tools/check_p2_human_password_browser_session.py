#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "auth": ROOT / "core/security/include/HumanPasswordBrowserAuthenticator.h",
    "account_h": ROOT / "core/security/include/HumanAccountRepository.h",
    "account_cpp": ROOT / "core/security/src/HumanAccountRepository.cpp",
    "provision_h": ROOT / "core/security/include/SecurityIdentityProvisioningRepository.h",
    "provision_cpp": ROOT / "core/security/src/SecurityIdentityProvisioningRepository.cpp",
    "gate_h": ROOT / "core/security/include/BrowserSessionHttpGate.h",
    "gate_cpp": ROOT / "core/security/src/BrowserSessionHttpGate.cpp",
    "security_gate": ROOT / "core/security/include/SecurityHttpGate.h",
    "server_h": ROOT / "core/http/include/TestHttpServer.h",
    "server_cpp": ROOT / "core/http/src/TestHttpServer.cpp",
    "test": ROOT / "core/security/tests/test_human_password_browser_session.cpp",
    "make": ROOT / "mk/security-sources.mk",
    "defaults": ROOT / "packaging/systemd/vdr-suite-daemon.default",
    "claim": ROOT / "core/security/src/FirstAdminClaimService.cpp",
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
        '"human-password"',
        "findByLogin(loginName)",
        "findCredential(verifier->credentialId)",
        "findByActorId(actor->actorId)",
        "ensureHumanBrowserDevice",
        "human-browser-",
        "crypt_r(",
        'passwordHash.rfind("$y$", 0)',
        'passwordHash.rfind("$6$", 0)',
        "AuthenticationState::Invalid",
        "AuthenticationState::Revoked",
    ):
        require("auth", marker)

    require("account_h", "findByActorId")
    require("account_cpp", "HumanAccountRepository::findByActorId")
    require("provision_h", "ensureHumanBrowserDevice")
    require(
        "provision_cpp",
        "SecurityIdentityProvisioningRepository::ensureHumanBrowserDevice",
    )

    for marker in (
        "HumanPasswordBrowserAuthenticator",
        "humanPasswordBrowserAuthenticator_",
    ):
        require("gate_h", marker)
        require("server_h", marker)

    for marker in (
        "humanPasswordBrowserAuthenticator_->authenticate",
        "humanContext.authenticationState !=",
        "AuthenticationState::Anonymous",
    ):
        require("gate_cpp", marker)

    gate = read("gate_cpp")
    human = gate.find("humanPasswordBrowserAuthenticator_->authenticate")
    managed = gate.find("managedBasicAuthenticator_->authenticate")
    if min(human, managed) < 0 or not human < managed:
        raise AssertionError(
            "human-password authentication must precede Managed Basic fallback"
        )
    forbid("gate_cpp", "legacyAuthenticator_->authenticate")
    forbid("gate_h", "LegacyBasicAuthenticator")

    forbid("security_gate", "HumanPasswordBrowserAuthenticator")
    require(
        "server_cpp",
        "std::make_unique<HumanPasswordBrowserAuthenticator>",
    )
    require("server_cpp", "humanPasswordBrowserAuthenticator_.get()")

    for marker in (
        "legacyCollision.rejection.statusCode == 401",
        '"invalid_credentials"',
        "response.statusCode == 200",
        'response.headers.count("Set-Cookie") == 1U',
        "csrfToken",
        "browser->issuedFromCredentialId",
        '"credential-human-admin"',
        "identities.revokeDevice(loginDeviceId)",
        "AuthenticationState::Revoked",
    ):
        require("test", marker)

    for marker in (
        "test-security-human-password-browser-session:",
        "test_human_password_browser_session.cpp",
        "python3 tools/check_p2_human_password_browser_session.py",
    ):
        require("make", marker)

    forbid("defaults", "VDR_SUITE_HUMAN_PASSWORD")
    require("defaults", "VDR_SUITE_SECURITY_MODE=enforced")
    forbid("claim", "HumanPasswordBrowserAuthenticator")

    audit = " ".join(read("audit").split())
    for marker in (
        "Human-password browser session bridge",
        "POST /api/security/browser-sessions",
        "known Human Account login owns its password decision",
        "human-browser-<accountId>",
        "next justified ADR-0066 slice is local audited Human Account recovery",
    ):
        if marker not in audit:
            raise AssertionError(
                f"{FILES['audit']} missing normalized marker: {marker}"
            )

    print("P2 human-password browser session contracts passed")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"P2 human-password browser session check failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
