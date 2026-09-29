#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "adr": ROOT / "docs/adr/ADR-0065-human-account-profile-device-identity-boundary.md",
    "audit": ROOT / "docs/development/post-phase69-p1-identity-model-audit.md",
    "index": ROOT / "docs/adr/index.md",
    "identity": ROOT / "core/security/include/SecurityIdentity.h",
    "config": ROOT / "core/security/include/SecurityConfiguration.h",
    "repo": ROOT / "core/security/src/SecurityIdentityRepository.cpp",
    "verifier": ROOT / "core/security/src/CredentialVerifierRepository.cpp",
    "grants": ROOT / "core/security/src/SecurityPermissionGrantRepository.cpp",
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
    require("identity", "enum class ActorType")
    require("identity", "User,")
    require("identity", "Service,")
    require("identity", "Agent,")
    require("repo", "security_actors")
    require("repo", "security_devices")
    require("repo", "security_sessions")
    require("repo", "security_credentials")
    require("verifier", "security_basic_credential_verifiers")
    require("grants", "security_actor_permission_grants")
    require("config", "SecurityMode::LegacyBasicCompatibility")
    require("config", '"legacy-basic"')
    require("config", '"Basic YWRtaW46dmRyLXN1aXRl"')

    for marker in (
        "Security Actor != Human Account",
        "Human Account != Profile",
        "Device trust != User identity",
        "Capability != Permission",
        "No Account endpoint is introduced until explicit Human Account persistence exists.",
        "unclaimed server",
        "yescrypt",
    ):
        require("adr", marker)

    for marker in (
        "ActorType::User does not mean Human Account",
        "No explicit Human Account entity",
        "admin:vdr-suite",
        "The missing multiuser product boundary is not security persistence",
        "Human Account --explicit binding--> User Actor",
    ):
        require("audit", marker)

    require("index", "ADR-0065-human-account-profile-device-identity-boundary.md")
    forbid("adr", "Actor == Human Account")
    print("P1 identity authority audit contracts passed")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"P1 identity authority audit check failed: {error}", file=sys.stderr)
        raise SystemExit(1)
