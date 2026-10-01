#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "config": ROOT / "core/security/include/SecurityConfiguration.h",
    "legacy": ROOT / "core/security/include/LegacyBasicAuthenticator.h",
    "server": ROOT / "core/http/src/TestHttpServer.cpp",
    "http_gate": ROOT / "core/security/include/SecurityHttpGate.h",
    "browser_gate_h": ROOT / "core/security/include/BrowserSessionHttpGate.h",
    "browser_gate_cpp": ROOT / "core/security/src/BrowserSessionHttpGate.cpp",
    "hbbtv": ROOT / "core/daemon/src/DaemonHbbtvRuntime.cpp",
    "identity_h": ROOT / "core/security/include/SecurityIdentityRepository.h",
    "identity_cpp": ROOT / "core/security/src/SecurityIdentityRepository.cpp",
    "config_test": ROOT / "core/security/tests/test_security_configuration.cpp",
    "http_test": ROOT / "core/security/tests/test_security_http_gate.cpp",
    "browser_test": ROOT / "core/security/tests/test_browser_session_http_gate.cpp",
    "adr": ROOT / "docs/adr/ADR-0066-unclaimed-server-first-admin-bootstrap-recovery.md",
    "audit": ROOT / "docs/development/post-phase69-p2-first-admin-bootstrap-audit.md",
    "architecture": ROOT / "docs/architecture/security-identity-foundation.md",
    "roadmap": ROOT / "docs/planning/roadmap.md",
    "root_roadmap": ROOT / "ROADMAP.md",
    "current": ROOT / "docs/CURRENT.md",
    "current_status": ROOT / "docs/development/current-status.md",
    "handoff": ROOT / "docs/NEW-CHAT-HANDOFF.md",
    "gap_matrix": ROOT / "docs/planning/architecture-audit-gap-matrix.md",
    "phase_map": ROOT / "docs/planning/phase-map.md",
    "account_adr": ROOT / "docs/adr/ADR-0065-human-account-profile-device-identity-boundary.md",
    "closeout": ROOT / "docs/development/post-phase69-p2-legacy-basic-retirement-closeout.md",
    "man": ROOT / "docs/man/man5/vdr-suite.conf.5",
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
    if FILES["legacy"].exists():
        raise AssertionError(
            "LegacyBasicAuthenticator.h still exists after retirement"
        )

    for marker in (
        "LegacyBasicCompatibility",
        "VDR_SUITE_BASIC_AUTH",
        "VDR_SUITE_LEGACY_BASIC_",
        "expectedAuthorizationHeader",
    ):
        forbid("config", marker)

    for name in ("server", "http_gate", "browser_gate_h", "browser_gate_cpp", "hbbtv"):
        forbid(name, "LegacyBasicAuthenticator")

    forbid("server", "ensureCompatibilityIdentity")
    forbid("http_gate", "usesLegacyCompatibilityCredential")
    forbid("browser_gate_cpp", "legacyAuthenticator_")
    forbid("hbbtv", "LegacyBasicCompatibility")
    forbid("identity_h", "ensureCompatibilityIdentity")
    forbid("identity_cpp", "ensureCompatibilityIdentity")

    for marker in (
        "retiredLegacyInputs",
        'setenv("VDR_SUITE_SECURITY_MODE", "legacy-basic", 1)',
        'setenv("VDR_SUITE_BASIC_AUTH", "Basic configured", 1)',
    ):
        require("config_test", marker)

    for marker in (
        "addRetiredLegacyAuthentication",
        "deniedLegacyOperationRead",
        "retiredLegacyUnmigrated",
    ):
        require("http_test", marker)

    for marker in (
        "kLegacyCredential",
        "retiredLegacyLogin",
        "managedLogin",
    ):
        require("browser_test", marker)

    for name, markers in {
        "adr": (
            "Authentication default migration and retirement completion",
            "There is therefore no runtime compatibility rollback",
            "retired Legacy Basic configuration is not a recovery authority",
        ),
        "audit": (
            "Legacy Basic runtime implementation removal",
            "Fresh package defaults no longer emit",
            "negative credential probe",
        ),
        "architecture": (
            "Legacy Basic runtime compatibility is now removed",
            "`LegacyBasicAuthenticator` is deleted",
            "old package-default lines may survive an upgrade",
        ),
        "roadmap": (
            "transitional Legacy Basic runtime implementation removed",
            "no Legacy Basic deployment mode or authenticator remains",
            "not a current rollback procedure",
            "Legacy Basic retirement [COMPLETED]",
        ),
        "root_roadmap": (
            "Legacy Basic retirement **[COMPLETED]**",
            "P2 Legacy Basic Retirement Closeout",
        ),
        "current": (
            "MU.0 through MU.5 are completed:",
            "Legacy Basic retirement.",
            "Post-Phase-69 P2 Legacy Basic Retirement Closeout",
        ),
        "current_status": (
            "The Multiuser foundation through MU.5 is completed:",
            "Legacy Basic retirement.",
            "P2 Legacy Basic Retirement Closeout",
        ),
        "handoff": (
            "Legacy Basic runtime compatibility is removed.",
            "Post-Phase-69 P2 Legacy Basic Retirement Closeout",
        ),
        "gap_matrix": (
            "| G-40 | Legacy Basic retirement | Closed deployment milestone |",
            "Next numbered runtime product domain — Phase 70",
        ),
        "phase_map": (
            "Legacy Basic retirement migration **[COMPLETED]**",
            "Post-Phase-69 P2 Legacy Basic Retirement Closeout",
        ),
        "account_adr": (
            "That migration and the subsequent runtime retirement are now completed",
            "security-default migration / Legacy Basic retirement boundary has since completed",
        ),
        "closeout": (
            "# Post-Phase-69 P2 Legacy Basic Retirement Closeout",
            "The cross-cutting **Legacy Basic retirement** product milestone is closed.",
            "No single one of those product milestones is made active merely by completing",
        ),
        "man": (
            "RETIRED SECURITY SETTINGS",
            "Legacy Basic compatibility has been removed",
            "cannot reactivate Legacy\nBasic authentication",
        ),
    }.items():
        for marker in markers:
            require(name, marker)

    for name, marker in (
        (
            "roadmap",
            "Legacy Basic compatibility remains transitional deployment compatibility.",
        ),
        (
            "root_roadmap",
            "- Legacy Basic retirement;",
        ),
        (
            "handoff",
            "Legacy Basic compatibility remains transitional and intentionally retained.",
        ),
        (
            "gap_matrix",
            "| G-40 | Legacy Basic retirement | Deferred deployment migration |",
        ),
    ):
        forbid(name, marker)

    print("P2 Legacy Basic runtime removal contracts passed")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"P2 Legacy Basic runtime removal check failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
