#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "config": ROOT / "core/security/include/SecurityConfiguration.h",
    "legacy": ROOT / "core/security/include/LegacyBasicAuthenticator.h",
    "server": ROOT / "core/http/src/TestHttpServer.cpp",
    "config_test": ROOT / "core/security/tests/test_security_configuration.cpp",
    "http_test": ROOT / "core/security/tests/test_security_http_gate.cpp",
    "browser_test": ROOT / "core/security/tests/test_browser_session_http_gate.cpp",
    "adr": ROOT / "docs/adr/ADR-0066-unclaimed-server-first-admin-bootstrap-recovery.md",
    "audit": ROOT / "docs/development/post-phase69-p2-first-admin-bootstrap-audit.md",
    "architecture": ROOT / "docs/architecture/security-identity-foundation.md",
    "roadmap": ROOT / "docs/planning/roadmap.md",
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

def main():
    for marker in (
        'if (mode == "enforced")',
        "configuration.expectedAuthorizationHeader.clear();",
        "configuration.grants.clear();",
        '"VDR_SUITE_BASIC_AUTH"',
        '"VDR_SUITE_LEGACY_BASIC_PERMISSIONS"',
    ):
        require("config", marker)

    for marker in (
        "configuration_.mode !=",
        "SecurityMode::LegacyBasicCompatibility",
        "return context;",
    ):
        require("legacy", marker)

    require(
        "server",
        "if (!configuration.expectedAuthorizationHeader.empty() &&",
    )

    for marker in (
        "enforcedWithLegacyEnvironment",
        "enforcedWithLegacyEnvironment.expectedAuthorizationHeader.empty()",
        "enforcedWithLegacyEnvironment.grants.empty()",
        'setenv("VDR_SUITE_SECURITY_MODE", "legacy-basic", 1)',
    ):
        require("config_test", marker)

    for marker in (
        "legacyRemoteDenied",
        "deniedLegacyOperationRead",
        "fixture.addBrowserAuthentication(remote, true)",
    ):
        require("http_test", marker)

    for marker in (
        "enforcedLegacyLogin",
        "enforcedManagedLogin",
        "enforced.mode = SecurityMode::Enforced",
    ):
        require("browser_test", marker)

    for name, markers in {
        "adr": (
            "hard Legacy Basic runtime boundary",
            "old compatibility secret cannot silently reactivate",
            "Only in that compatibility mode",
        ),
        "audit": (
            "Enforced-mode Legacy Basic runtime fence",
            "legacy-basic -> enforced -> legacy-basic",
            "deletion of the compatibility implementation",
        ),
        "architecture": (
            "Explicit `enforced` mode now disables the Legacy Basic adapter",
            "compatibility inputs effective again",
        ),
        "roadmap": (
            "enforced runtime fence implemented",
            "no longer authenticates Legacy Basic",
            "restore the deployment to `enforced`",
        ),
        "man": (
            "the Legacy Basic authenticator is",
            "Stale compatibility values",
            "only then do those compatibility variables become",
        ),
    }.items():
        for marker in markers:
            require(name, marker)

    print("P2 enforced Legacy Basic retirement contracts passed")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"P2 enforced Legacy Basic retirement check failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
