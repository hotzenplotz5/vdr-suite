#!/usr/bin/env python3
"""MU.10C1: keep one-shot device issuance inside canonical security boundaries."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = {
    "service": "core/security/src/DevicePairingRequestService.cpp",
    "pairing": "core/security/src/DevicePairingRequestRepository.cpp",
    "verifier": "core/security/src/DeviceCredentialVerifierRepository.cpp",
    "api": "api/rest/src/PublicApiRuntime.cpp",
    "gate": "core/security/include/SecurityHttpGate.h",
    "server": "core/http/src/TestHttpServer.cpp",
    "scope": "docs/development/post-phase69-mu10c-device-credential-issuance.md",
}
text = {key: (ROOT / path).read_text(encoding="utf-8")
        for key, path in FILES.items()}

def require(key, needle):
    if needle not in text[key]:
        raise AssertionError(f"{FILES[key]} missing {needle!r}")

def forbid(key, needle):
    if needle in text[key]:
        raise AssertionError(f"{FILES[key]} contains forbidden {needle!r}")

def main():
    for needle in (
        "ActorType::Service",
        "provisioning.ensureTechnicalIdentity(",
        "verifiers.insertInActiveTransaction(",
        "repository_.consumeApprovedInActiveTransaction(",
        "accountabilityRepository_.append(event)",
        "transaction.commit()",
        "verifySecret(request.pairingToken,",
        "issued.credentialSecret",
    ):
        require("service", needle)

    for needle in (
        "BEGIN IMMEDIATE",
        "device.pairing.bootstrap",
        '"device-app"',
    ):
        require("service", needle)

    for needle in (
        "AND state = 'approved'",
        "AND invalidated_at = ''",
        "AND expires_at > CURRENT_TIMESTAMP",
        "revision = revision + 1",
        "state = 'consumed'",
    ):
        require("pairing", needle)

    for needle in (
        "security_device_credential_verifiers",
        "FOREIGN KEY(credential_id)",
        "FOREIGN KEY(device_id)",
        "INSERT INTO security_device_credential_verifiers",
        "transactionActive()",
    ):
        require("verifier", needle)

    for key in ("service", "verifier"):
        for forbidden in (
            "security_actor_permission_grants",
            "ensureGrant(",
            "security_human_accounts",
            "security_browser_session_credentials",
            "INSERT INTO security_sessions",
        ):
            forbid(key, forbidden)

    require("api", 'const std::string issueSuffix = "/credential"')
    require("api", 'response.headers["Cache-Control"] = "no-store"')
    require("api", '"pairing_consumed"')
    require("gate", "isPublicDeviceCredentialIssue")
    require("gate", "hasPublicDevicePairingToken")
    require("server", "registerDeviceCredentialIssue(")
    require("scope", "MU.10D")
    require("scope", "No permission-grant or Session write")

    print("MU.10C Device credential issuance architecture guard passed")
    print("MU10C1=ISSUANCE_CANDIDATE_NOT_REAL_YAVDR_ACCEPTED")
    print("PHASE70=NOT_STARTED")
    return 0

if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as error:
        print(f"MU.10C architecture guard failed: {error}")
        raise SystemExit(1)
