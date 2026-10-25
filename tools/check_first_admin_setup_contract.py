#!/usr/bin/env python3
"""Fail-closed static contract check for the local first-admin CLI.

Runtime claim/rollback/replay coverage lives in the existing
test-security-first-admin-claim-service target. This checker prevents
the helper from creating a second HTTP/authorization path.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "apps/tools/first_admin_setup.cpp").read_text()
security_mk = (ROOT / "mk/security-sources.mk").read_text()
install_mk = (ROOT / "mk/install.mk").read_text()
debian = (ROOT / "debian/vdr-suite-daemon.install").read_text()
guide = (ROOT / "docs/erste-schritte-nach-installation.md").read_text()

required = (
    "geteuid() != 0",
    "isatty(STDIN_FILENO)",
    "isatty(STDOUT_FILENO)",
    "tcgetattr",
    "tcsetattr",
    "~ECHO",
    "secureWipe(password)",
    "FirstAdminBootstrapIssuanceService issuer",
    "FirstAdminClaimService claimer",
    "issuer.issue()",
    "claimer.claim(std::move(request))",
    "FirstAdminClaimState::unclaimed",
    "issued.bootstrap->clearSecret()",
    "request.setupSecret = std::move",
    "std::move(password)",
)
for item in required:
    if item not in source:
        raise SystemExit(f"interactive setup missing required fence: {item}")
for forbidden in ("system(", "popen(", "curl ", "http://", "https://", "std::getenv(", "std::cout << issued", "request.password = argv", "request.setupSecret = argv"):
    if forbidden in source:
        raise SystemExit(f"interactive setup contains forbidden shortcut: {forbidden}")
for name, value in (
    ("make", security_mk),
    ("install", install_mk),
    ("debian manifest", debian),
    ("install guide", guide),
):
    if "vdr-suite-first-admin-setup" not in value:
        raise SystemExit(f"{name} does not reference interactive setup")
print("FIRST_ADMIN_INTERACTIVE_SETUP_CONTRACT=PASS")
