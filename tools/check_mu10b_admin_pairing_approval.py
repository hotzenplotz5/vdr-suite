#!/usr/bin/env python3
"""Guard the bounded MU.10B Administrator Pairing Approval slice."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")

def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)

repo = read("core/security/src/DevicePairingRequestRepository.cpp")
service = read("core/security/src/DevicePairingRequestService.cpp")
public = read("api/rest/src/PublicApiRuntime.cpp")
gate = read("core/security/include/SecurityHttpGate.h")
server = read("core/http/src/TestHttpServer.cpp")

for token in (
    "revision INTEGER NOT NULL DEFAULT 1",
    "decided_by_actor_id",
    "decided_at",
    "AND state = 'pending'",
    "AND expires_at > CURRENT_TIMESTAMP",
):
    require(token in repo, "MU.10B persistence guard missing: " + token)

for token in (
    "listPendingForAdministration",
    "readForAdministration",
    "DevicePairingRequestService::decide",
    "device_pairing.administration",
    'event.permission = "role.admin"',
    '"pairing_request_approved"',
    '"pairing_request_rejected"',
):
    require(token in service, "MU.10B service guard missing: " + token)

for forbidden in (
    "ensureIdentity(",
    "ensureGrant(",
    "BrowserSession",
    "ManagedBasic",
    "security_devices",
    "security_credentials",
    "security_sessions",
    "security_actor_permission_grants",
):
    require(forbidden not in service,
            "MU.10B crossed identity/credential/grant boundary: " + forbidden)

for token in (
    "isPublicDevicePairingBootstrapCreate",
    "isPublicDevicePairingBootstrapPoll",
    "isPublicDevicePairingAdminRead",
    "isPublicDevicePairingDecision",
    'pairingReadRequest.permission = "role.admin"',
    'requestToAuthorize.permission =\n                "role.admin"',
):
    require(token in gate, "MU.10B security gate guard missing: " + token)

for token in (
    "public-api.device-pairing-administration",
    "PublicDevicePairingAdministrationStatus",
    "publicDevicePairingAdministrationResponse",
    "precondition_required",
    "publicDevicePairingRevision",
    "revision_conflict",
    "operation_conflict",
):
    require(token in public, "MU.10B Public-v1 guard missing: " + token)

require(
    "actor->type != ActorType::User" in server and
    "registerDevicePairingDecision" in server,
    "MU.10B runtime must bind decisions to an active human User actor",
)

print("MU.10B Administrator Pairing Approval guard passed.")
print("Boundary: approval/rejection changes only the Pairing Request lifecycle.")
print("No Device/Credential/Session/Permission Grant is issued by MU.10B.")
