#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "adr": ROOT / "docs/adr/ADR-0066-unclaimed-server-first-admin-bootstrap-recovery.md",
    "audit": ROOT / "docs/development/post-phase69-p2-first-admin-bootstrap-audit.md",
    "index": ROOT / "docs/adr/index.md",
    "config": ROOT / "core/security/include/SecurityConfiguration.h",
    "defaults": ROOT / "packaging/systemd/vdr-suite-daemon.default",
    "unit": ROOT / "packaging/systemd/vdr-suite-daemon.service",
    "provisioning": ROOT / "core/security/src/SecurityIdentityProvisioningRepository.cpp",
    "verifier": ROOT / "core/security/src/CredentialVerifierRepository.cpp",
    "account": ROOT / "core/security/src/HumanAccountRepository.cpp",
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
    forbid("config", '"legacy-basic"')
    forbid("config", "LegacyBasicCompatibility")
    forbid("config", "VDR_SUITE_BASIC_AUTH")
    forbid("config", "VDR_SUITE_LEGACY_BASIC_")
    require("config", "VDR_SUITE_MANAGED_BASIC_USERNAME")
    require("unit", "EnvironmentFile=-/etc/default/vdr-suite-daemon")
    require("provisioning", "INSERT OR IGNORE INTO security_actors")
    require("verifier", "security_basic_credential_verifiers")
    require("account", "security_human_accounts")

    for marker in (
        "unclaimed",
        "claimed",
        "short-lived",
        "single-use",
        "one-way verifier",
        "local trusted operator",
        "Local recovery",
        "Legacy Basic retirement is a separate migration slice.",
        "A partially created first admin must not survive failed claim completion.",
    ):
        require("adr", marker)

    for marker in (
        "Current main does not provide a canonical lifecycle",
        "The remaining blocker to retiring the compatibility default is not missing password verification.",
        "persistent claim/bootstrap state",
        "no authentication-default change yet",
    ):
        require("audit", marker)

    require("adr", "Accepted architecture.")
    require("audit", "With ADR-0066 accepted")

    index = read("index")
    active_marker = "## Active Canonical ADRs"
    proposed_marker = "## Proposed Canonical ADRs"
    superseded_marker = "## Superseded Canonical ADRs"
    if active_marker not in index or proposed_marker not in index or superseded_marker not in index:
        raise AssertionError("ADR index is missing canonical status sections")
    active = index.split(active_marker, 1)[1].split(proposed_marker, 1)[0]
    proposed = index.split(proposed_marker, 1)[1].split(superseded_marker, 1)[0]
    adr_link = "ADR-0066-unclaimed-server-first-admin-bootstrap-recovery.md"
    if adr_link not in active:
        raise AssertionError("ADR-0066 is not listed as active")
    if adr_link in proposed:
        raise AssertionError("ADR-0066 is still listed as proposed")

    forbid("adr", "plaintext bootstrap secret persistence is allowed")
    forbid("adr", "Remote anonymous recovery is allowed")

    print("P2 first-admin bootstrap boundary contracts passed")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"P2 first-admin bootstrap boundary check failed: {error}", file=sys.stderr)
        raise SystemExit(1)
