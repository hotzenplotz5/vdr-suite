#!/usr/bin/env python3
"""Guard the bounded post-Phase-69 MU.10A Device Pairing bootstrap slice."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")

def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)

repo_cpp = read("core/security/src/DevicePairingRequestRepository.cpp")
service_h = read("core/security/include/DevicePairingRequestService.h")
service_cpp = read("core/security/src/DevicePairingRequestService.cpp")
public_h = read("api/rest/include/PublicApiRuntime.h")
public_cpp = read("api/rest/src/PublicApiRuntime.cpp")
gate = read("core/security/include/SecurityHttpGate.h")
server = read("core/http/src/TestHttpServer.cpp")
client = read("clients/reference-js/public-v1-client.js")

for token in (
    "security_device_pairing_requests",
    "user_code_hash",
    "pairing_token_hash",
    "state TEXT NOT NULL DEFAULT 'pending'",
    "expires_at",
):
    require(token in repo_cpp, "pairing persistence contract drifted: " + token)

for forbidden in (
    "user_code TEXT",
    "pairing_token TEXT",
    "security_devices (",
    "security_credentials (",
    "security_sessions (",
    "security_actor_permission_grants (",
):
    require(forbidden not in repo_cpp, "MU.10A invented forbidden persistence: " + forbidden)

for token in (
    "getrandom(",
    "crypt_r(",
    "LifetimeSeconds = 600",
    "PollIntervalSeconds = 3",
    "MaxActivePendingRequests = 256U",
    "ExpiredRetentionSeconds = 3600",
    "device_pairing.requested",
    "pairing_request_created",
    "DevicePairingPollStatus::unauthorized",
    "DevicePairingPollStatus::expired",
):
    require(token in service_cpp or token in service_h,
            "pairing service contract drifted: " + token)

for forbidden in (
    "ensureIdentity(",
    "ensureGrant(",
    "BrowserSession",
    "ManagedBasic",
    "human-password",
):
    require(forbidden not in service_cpp,
            "MU.10A crossed identity/grant/browser boundary: " + forbidden)

for token in (
    '"/api/v1/device-pairings"',
    '"/api/v1/device-pairings/"',
    "PublicDevicePairingCreateStatus",
    "PublicDevicePairingLookupStatus",
    "public-api.device-pairing-bootstrap",
    "pairing_expired",
    "rate_limited",
):
    require(token in public_h or token in public_cpp,
            "public Pairing contract drifted: " + token)

require(
    '"X-VDR-Suite-Pairing-Token"' in server,
    "HTTP server must forward the short-lived Pairing polling token",
)
require(
    "isPublicDevicePairingBootstrapCreate" in gate and
    "isPublicDevicePairingBootstrapPoll" in gate and
    '"X-VDR-Suite-Pairing-Token"' in gate,
    "Pairing bootstrap create/poll routes must be explicitly and narrowly admitted by the security gate",
)
require(
    'path == "/api/v1/backends"' in gate and
    "if (!gate.context.authenticated())" in gate,
    "ordinary Public-v1 authentication boundary must remain enforced",
)

for token in (
    "createDevicePairing(options)",
    "getDevicePairing(options)",
    "'X-VDR-Suite-Pairing-Token'",
    "'/api/v1/device-pairings'",
):
    require(token in client, "reference client Pairing seam drifted: " + token)

for forbidden in (
    "setInterval(",
    "setTimeout(",
    "/api/security/browser-sessions",
    "Basic ",
):
    require(forbidden not in client,
            "reference client invented polling/auth fallback: " + forbidden)

inventory = read("tools/check_phase69_public_api_inventory.py")
for token in (
    '"/api/v1/device-pairings"',
    '"/api/v1/device-pairings/"',
):
    require(token in inventory,
            "public route inventory must classify MU.10A route: " + token)

print("MU.10A Device Pairing bootstrap guard passed.")
print("Boundary: short-lived Pairing Request + token-scoped polling only.")
print("No Actor/Device/Credential/Session/Grant issuance is owned by MU.10A.")
