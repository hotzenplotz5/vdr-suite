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
    require("defaults", "VDR_SUITE_SECURITY_MODE=enforced")
    forbid("defaults", "\nVDR_SUITE_SECURITY_MODE=legacy-basic\n")
    forbid("defaults", "VDR_SUITE_BASIC_AUTH=")
    forbid("defaults", "VDR_SUITE_MANAGED_BASIC_PASSWORD_HASH=")

    for marker in (
        '"VDR_SUITE_SECURITY_MODE"',
        '"legacy-basic"',
        'SecurityMode::LegacyBasicCompatibility',
        'SecurityMode::Enforced',
    ):
        require("config", marker)

    for marker in (
        'setenv("VDR_SUITE_SECURITY_MODE", "enforced", 1)',
        'setenv("VDR_SUITE_SECURITY_MODE", "legacy-basic", 1)',
        "explicitCompatibilityRollback",
        "SecurityMode::LegacyBasicCompatibility",
    ):
        require("config_test", marker)

    for marker in (
        "test -e $(DESTDIR)$(SYSCONFDIR)/default/vdr-suite-daemon ||",
        "grep -Fx 'VDR_SUITE_SECURITY_MODE=enforced'",
        "VDR_SUITE_UPGRADE_SENTINEL=preserve",
        "grep -Fx 'VDR_SUITE_SECURITY_MODE=legacy-basic'",
        "$(MAKE) install-systemd DESTDIR=/tmp/vdr-suite-pkgroot PREFIX=/usr",
    ):
        require("install", marker)

    for marker in (
        "Fresh packaged installations",
        "/etc/default/vdr-suite-daemon",
        "VDR_SUITE_SECURITY_MODE=legacy-basic",
        "code fallback remains",
        "legacy-basic",
    ):
        require("adr", marker)

    for marker in (
        "Authentication-default fresh-install migration",
        "fresh install",
        "upgrade",
        "rollback",
        "VDR_SUITE_SECURITY_MODE=enforced",
    ):
        require("audit", marker)

    for marker in (
        "Fresh packaged installations",
        "upgrade compatibility",
        "full Legacy Basic removal remains deferred",
    ):
        require("architecture", marker)

    for marker in (
        "Fresh packaged installations now select",
        "existing deployment",
        "default files are preserved",
        "real deployment rollback",
    ):
        require("roadmap", marker)

    for marker in (
        "VDR_SUITE_SECURITY_MODE",
        "Fresh packaged installations default to",
        "legacy-basic",
        "enforced",
    ):
        require("man", marker)

    print("P2 authentication-default migration contracts passed")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"P2 authentication-default migration check failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
