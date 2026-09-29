#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

errors = []

def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        errors.append(f"missing required file: {path}")
        return ""
    return target.read_text(encoding="utf-8")

def require(path: str, needle: str) -> None:
    text = read(path)
    if needle not in text:
        errors.append(f"{path}: missing required marker: {needle!r}")

adr = "docs/adr/ADR-0065-human-account-profile-device-identity-boundary.md"
audit = "docs/development/post-phase69-p1-identity-model-audit.md"

for marker in (
    "One persistent identity authority",
    "Security Actor is not Human Account",
    "Human Account is not Profile",
    "Credential, Session and Device remain distinct",
    "Capability never grants permission",
    "Fresh-install bootstrap",
    "Pairing reuses, rather than replaces, identity",
    "must not synthesize Accounts from every `ActorType::User` row",
):
    require(adr, marker)

for marker in (
    "security_actors",
    "security_devices",
    "security_sessions",
    "security_credentials",
    "security_basic_credential_verifiers",
    "security_actor_permission_grants",
    "security_browser_session_credentials",
    "accountability_events",
    "admin:vdr-suite",
    "Actor != automatically Human Account",
    "Human Account != Profile",
    "Device trust != User identity",
    "Capability != Permission",
):
    require(audit, marker)

require("docs/adr/ADR-0013-permission-model.md", "Actor is intentionally broader than User.")
require("core/security/src/SecurityIdentityRepository.cpp", "CREATE TABLE IF NOT EXISTS security_actors")
require("core/security/src/SecurityIdentityRepository.cpp", "CREATE TABLE IF NOT EXISTS security_devices")
require("core/security/src/SecurityIdentityRepository.cpp", "CREATE TABLE IF NOT EXISTS security_sessions")
require("core/security/src/SecurityIdentityRepository.cpp", "CREATE TABLE IF NOT EXISTS security_credentials")
require("core/security/src/CredentialVerifierRepository.cpp", "security_basic_credential_verifiers")
require("core/security/src/SecurityPermissionGrantRepository.cpp", "security_actor_permission_grants")
require("core/security/src/BrowserSessionCredentialRepository.cpp", "security_browser_session_credentials")
require("core/security/src/AccountabilityEventRepository.cpp", "accountability_events")
require("core/security/include/SecurityConfiguration.h", "SecurityMode::LegacyBasicCompatibility")
require("core/security/include/SecurityConfiguration.h", "Basic YWRtaW46dmRyLXN1aXRl")
require("packaging/systemd/vdr-suite-daemon.service", "EnvironmentFile=-/etc/default/vdr-suite-daemon")
require("packaging/systemd/vdr-suite-daemon.service", "VDR_SUITE_DATABASE_PATH=/var/lib/vdr-suite/vdr-suite.db")
require("core/security/include/ManagedBasicAuthenticator.h", 'passwordHash.rfind("$y$", 0) == 0')
require("core/security/include/ManagedBasicAuthenticator.h", 'passwordHash.rfind("$6$", 0) == 0')
require("core/security/include/BrowserSessionIssuanceService.h", "DefaultLifetimeSeconds = 28800")

index = read("docs/adr/index.md")
if "ADR-0065: Human Account, Profile and Device Identity Boundary" not in index:
    errors.append("ADR index does not list ADR-0065")
if "Canonical ADR sequence currently runs through:\n\n```text\nADR-0065" not in index:
    errors.append("ADR index canonical sequence does not run through ADR-0065")
if "Next available canonical ADR:\n\n```text\nADR-0066" not in index:
    errors.append("ADR index next available canonical number is not ADR-0066")

if errors:
    for error in errors:
        print("P1 identity model audit check:", error, file=sys.stderr)
    raise SystemExit(1)

print("P1 identity model audit architecture contract ok")
