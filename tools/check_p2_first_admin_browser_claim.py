#!/usr/bin/env python3
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

FILES = {
    "http_h": ROOT / "core/http/include/FirstAdminClaimHttpService.h",
    "http_cpp": ROOT / "core/http/src/FirstAdminClaimHttpService.cpp",
    "http_test": ROOT / "core/http/tests/test_first_admin_claim_http_service.cpp",
    "server_h": ROOT / "core/http/include/TestHttpServer.h",
    "server_cpp": ROOT / "core/http/src/TestHttpServer.cpp",
    "claim_cpp": ROOT / "core/security/src/FirstAdminClaimService.cpp",
    "security_gate": ROOT / "core/security/include/SecurityHttpGate.h",
    "browser_gate": ROOT / "core/security/src/BrowserSessionHttpGate.cpp",
    "security_make": ROOT / "mk/security-sources.mk",
    "runtime_make": ROOT / "mk/runtime-api-tests.mk",
    "daemon_make": ROOT / "mk/daemon-sources.mk",
    "defaults": ROOT / "packaging/systemd/vdr-suite-daemon.default",
    "adr66": ROOT / "docs/adr/ADR-0066-unclaimed-server-first-admin-bootstrap-recovery.md",
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
    route = "/api/security/first-admin/claim"

    for marker in (
        route,
        "FirstAdminClaimService& claimService",
        "claimService_.claim(std::move(claimRequest))",
        "application/json",
        "Cache-Control",
        "X-Request-ID",
        "X-Correlation-ID",
        "FirstAdminClaimStatus::bootstrapExpired",
        "FirstAdminClaimStatus::bootstrapConsumed",
        "FirstAdminClaimStatus::bootstrapInvalidated",
        "FirstAdminClaimStatus::claimed",
    ):
        require("http_cpp", marker)

    for marker in (
        "FirstAdminClaimService",
        "FirstAdminClaimHttpService",
        "HumanAccountRepository",
        "FirstAdminBootstrapRepository",
    ):
        require("server_h", marker)

    for marker in (
        "firstAdminClaimService_ =",
        "firstAdminClaimHttpService_ =",
        "firstAdminClaimHttpService_->handles(request)",
        "firstAdminClaimHttpService_->handle(request)",
    ):
        require("server_cpp", marker)

    server = read("server_cpp")
    claim_position = server.find(
        "firstAdminClaimHttpService_->handles(request)"
    )
    browser_position = server.find(
        "browserSessionHttpGate_->handles(request)"
    )
    normal_position = server.find(
        "securityHttpGate_->evaluate(request)"
    )
    if min(claim_position, browser_position, normal_position) < 0:
        raise AssertionError("missing HTTP gate ordering marker")
    if not claim_position < browser_position < normal_position:
        raise AssertionError(
            "first-admin claim must be isolated before normal auth gates"
        )

    for marker in (
        "BrowserSessionIssuanceService",
        "Set-Cookie",
        "csrf",
        "/api/v1",
        "Authorization",
    ):
        forbid("http_cpp", marker)

    forbid("security_gate", route)
    forbid("browser_gate", route)
    forbid("browser_gate", "FirstAdminClaim")
    require(
        "claim_cpp",
        "safeText(request.correlationId, 128, 0)",
    )

    for marker in (
        "response.statusCode == 201",
        "invalid_bootstrap_proof",
        "bootstrap_expired",
        "bootstrap_consumed",
        "bootstrap_invalidated",
        "server_already_claimed",
        "fixture.accounts.listAll().accounts.empty()",
        "ActorType::User",
        '"role.admin"',
        '"security.first-admin.claim"',
        'response.headers.count("Set-Cookie") == 0U',
        'other.path = "/api/v1/accounts"',
        "assert(!fixture.http->handles(other))",
    ):
        require("http_test", marker)

    for marker in (
        "FIRST_ADMIN_HTTP_SRC",
        "test-security-first-admin-claim-http-service:",
        "python3 tools/check_p2_first_admin_browser_claim.py",
    ):
        require("security_make", marker)

    require("runtime_make", "$(FIRST_ADMIN_HTTP_SRC)")
    require(
        "daemon_make",
        "core/http/src/FirstAdminClaimHttpService.cpp",
    )

    forbid("defaults", "VDR_SUITE_FIRST_ADMIN")
    forbid("defaults", "VDR_SUITE_SECURITY_MODE=")
    forbid("defaults", "VDR_SUITE_BASIC_AUTH=")
    forbid("defaults", "VDR_SUITE_LEGACY_BASIC_")

    for marker in (
        "trusted browser completion",
        "Bootstrap material itself is not a browser session",
    ):
        require("adr66", marker)

    audit = " ".join(read("audit").split())
    for marker in (
        "Claim-only browser completion boundary",
        route,
        "does not create a Browser Session",
        "active Device",
        "Legacy Basic runtime implementation removal",
        "normal human-password browser authentication",
    ):
        if marker not in audit:
            raise AssertionError(
                f"{FILES['audit']} missing normalized marker: {marker}"
            )

    print("P2 first-admin browser claim contracts passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(
            f"P2 first-admin browser claim check failed: {error}",
            file=sys.stderr,
        )
        raise SystemExit(1)
