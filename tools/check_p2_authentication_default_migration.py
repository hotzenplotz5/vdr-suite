#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "defaults": ROOT / "packaging/systemd/vdr-suite-daemon.default",
    "config": ROOT / "core/security/include/SecurityConfiguration.h",
    "config_test": ROOT / "core/security/tests/test_security_configuration.cpp",
    "install": ROOT / "mk/install.mk",
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

def forbid(name, marker):
    if marker in read(name):
        raise AssertionError(f"{FILES[name]} contains forbidden marker: {marker}")

def main():
    for marker in (
        "\nVDR_SUITE_SECURITY_MODE=",
        "\nVDR_SUITE_BASIC_AUTH=",
        "\nVDR_SUITE_LEGACY_BASIC_",
        "\nVDR_SUITE_MANAGED_BASIC_PASSWORD_HASH=",
    ):
        forbid("defaults", marker)

    for marker in (
        '"VDR_SUITE_SECURITY_MODE"',
        '"legacy-basic"',
        "LegacyBasicCompatibility",
        "SecurityMode::Enforced",
        "VDR_SUITE_BASIC_AUTH",
        "VDR_SUITE_LEGACY_BASIC_",
    ):
        forbid("config", marker)

    for marker in (
        '"VDR_SUITE_MANAGED_BASIC_USERNAME"',
        '"VDR_SUITE_MANAGED_BASIC_PASSWORD_HASH"',
    ):
        require("config", marker)

    for marker in (
        "retiredLegacyInputs",
        'setenv("VDR_SUITE_SECURITY_MODE", "legacy-basic", 1)',
        'setenv("VDR_SUITE_BASIC_AUTH", "Basic configured", 1)',
    ):
        require("config_test", marker)

    for marker in (
        "test -e $(DESTDIR)$(SYSCONFDIR)/default/vdr-suite-daemon ||",
        "! grep -F 'VDR_SUITE_SECURITY_MODE='",
        "! grep -F 'VDR_SUITE_BASIC_AUTH='",
        "! grep -F 'VDR_SUITE_LEGACY_BASIC_'",
        "VDR_SUITE_UPGRADE_SENTINEL=preserve",
        "grep -Fx 'VDR_SUITE_SECURITY_MODE=legacy-basic'",
        "$(MAKE) install-systemd DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr",
    ):
        require("install", marker)

    for marker in (
        "Authentication default migration and retirement completion",
        "no runtime compatibility rollback",
        "historical lines may remain but are inert",
    ):
        require("adr", marker)

    for marker in (
        "Legacy Basic runtime implementation removal",
        "Fresh package defaults no longer emit",
        "negative credential probe",
    ):
        require("audit", marker)

    for marker in (
        "Legacy Basic runtime compatibility is now removed",
        "old package-default lines may survive an upgrade",
        "Historical retirement acceptance evidence",
    ):
        require("architecture", marker)

    for marker in (
        "transitional Legacy Basic runtime implementation removed",
        "stale `VDR_SUITE_SECURITY_MODE`",
        "not a current rollback procedure",
    ):
        require("roadmap", marker)

    for marker in (
        "RETIRED SECURITY SETTINGS",
        "Legacy Basic compatibility has been removed",
        "values are inert",
    ):
        require("man", marker)

    print("P2 authentication-default retirement contracts passed")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"P2 authentication-default retirement check failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
