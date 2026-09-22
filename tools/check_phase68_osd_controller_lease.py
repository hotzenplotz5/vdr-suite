#!/usr/bin/env python3
"""Phase 68.F exclusive Legacy OSD controller-lease guard."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

paths = {
    "domain": ROOT / "core/vdr/include/LegacyOsdControllerDomain.h",
    "service_h": ROOT / "core/daemon/include/OsdControllerLeaseService.h",
    "service": ROOT / "core/daemon/src/OsdControllerLeaseService.cpp",
    "viewer": ROOT / "core/vdr/include/LegacyOsdViewerDomain.h",
    "api": ROOT / "api/rest/src/LegacyOsdApiRuntime.cpp",
    "daemon": ROOT / "core/daemon/src/DaemonLegacyOsdRuntime.cpp",
    "runtime": ROOT / "core/daemon/src/DaemonRuntime.cpp",
    "security": ROOT / "core/security/include/SecurityHttpGate.h",
    "auth": ROOT / "core/security/include/AuthorizationService.h",
    "plugin_caps": ROOT / "vdr-plugin-suite-bridge/suitebridge_capabilities.cpp",
}

errors = []
contents = {}
for label, path in paths.items():
    if not path.is_file():
        errors.append(f"missing Phase-68.F file: {path.relative_to(ROOT)}")
        contents[label] = ""
    else:
        contents[label] = path.read_text(encoding="utf-8")

required = {
    "domain": [
        "struct OsdControllerLease",
        "controllerLeaseId",
        "leaseRevision",
        "controllerLeaseEpoch",
        "legacyOsdSessionId",
        "viewerBindingId",
        "actorId",
        "clientInstanceId",
        "backendId",
        "backendGeneration",
        "osdSurfaceId",
        "osdEpoch",
        "grantedAt",
        "expiresAt",
        "renewAfter",
        "lastHeartbeatAt",
        "OsdControllerLeaseState",
        "revocationReason",
        "Requested",
        "Active",
        "Expiring",
        "Revoked",
        "Expired",
        "Released",
    ],
    "service_h": [
        "LeaseLifetimeSeconds",
        "RenewAfterSeconds",
        "MaximumLeases",
        "MaximumSurfaceScopes",
        "OsdControllerLeaseResult acquire(",
        "OsdControllerLeaseResult renew(",
        "OsdControllerLeaseResult release(",
        "OsdControllerLeaseResult current(",
        "OsdControllerLeaseResult revoke(",
    ],
    "service": [
        'request.permission = "osd.control"',
        "sessionService_.status(",
        "viewerService_.find(",
        "backendPolicyLookup_",
        "read_only_backend",
        "controller_lease_conflict",
        "generation_conflict",
        "revision_conflict",
        "legacy_osd_controller_surface_changed",
        "legacy_osd_controller_epoch_changed",
        "surfaceOwners_",
        "surfaceEpochs_",
    ],
    "api": [
        '"/api/vdr/legacy-osd/controller-leases"',
        '"/api/vdr/legacy-osd/controller-leases/renew"',
        '"/api/vdr/legacy-osd/controller-leases/release"',
        '"/api/vdr/legacy-osd/controller-leases/status"',
        '"Cache-Control"] = "no-store"',
        "controllerLeaseJson(",
        "controllerStatusJson(",
    ],
    "daemon": [
        "std::unique_ptr<OsdControllerLeaseService>",
        "std::make_unique<OsdControllerLeaseService>",
        "BackendRegistryService& backendRegistryService",
        "BackendAccessPolicy& backendAccessPolicy",
        "backendAccessPolicy.canWriteToBackend(",
        "*sessions, *viewers, *controllers",
    ],
    "runtime": [
        "*backendRegistryService_",
        "*backendAccessPolicy_",
    ],
    "security": [
        'requestToAuthorize.permission = "osd.control"',
        '"osd.controller.acquire"',
        '"osd.controller.renew"',
        '"osd.controller.release"',
        '"osd.controller.status"',
        "isLegacyOsdControllerMutation",
    ],
    "viewer": [
        "controlAuthorized = false",
    ],
    "plugin_caps": [
        '{"osd.control", SuiteBridgeCapabilityState::Available}',
    ],
}

for label, tokens in required.items():
    for token in tokens:
        if token not in contents[label]:
            errors.append(f"{label} missing required token: {token}")

auth = contents["auth"]
mutating_start = auth.find("static bool mutatingPermission")
mutating_end = auth.find("static bool isReadOnlyMutation", mutating_start)
mutating_block = auth[mutating_start:mutating_end]
if 'permission == "osd.control"' not in mutating_block:
    errors.append("osd.control must participate in role.read-only mutation denial")

admin_start = auth.find("static bool adminRoleGrants")
admin_end = auth.find("static bool mutatingPermission", admin_start)
admin_block = auth[admin_start:admin_end]
if "osd.control" in admin_block:
    errors.append("role.admin must not synthesize osd.control")

security = contents["security"]
protected_start = security.find("const bool isProtectedMutation")
protected_end = security.find("const bool isExplicitlyAuthorizedPost", protected_start)
if "isLegacyOsdControllerMutation" not in security[protected_start:protected_end]:
    errors.append("controller lease lifecycle must remain a protected mutation")

api = contents["api"]
for forbidden_route in (
    "/api/vdr/legacy-osd/" + "key",
    "/api/vdr/legacy-osd/" + "press",
    "/api/vdr/legacy-osd/" + "remote",
):
    if forbidden_route in api or forbidden_route in security:
        errors.append(
            "Phase-68.F introduced a Phase-68.G input route: "
            + forbidden_route
        )

controller_lease_only = "\n".join(
    contents[label] for label in
    ("domain", "service_h", "service")
)
for forbidden in (
    "OsdInput" + "Command",
    "OsdInput" + "Result",
    "c" + "Remote",
    "remote." + "Put",
    "pressRaw" + "Key",
    "run" + "Svdrp",
    "call" + "Plugin",
    "SVDRP" + "Command",
):
    if forbidden in controller_lease_only:
        errors.append(
            "Phase-68.F controller lease owner absorbed native input dispatch: "
            + forbidden
        )

for forbidden_payload in (
    "PRIVATE_CONTROLLER_FRAME_TEXT",
    "frame.title",
    "frame.items",
):
    if forbidden_payload in contents["api"] or        forbidden_payload in contents["daemon"] or        forbidden_payload in contents["security"]:
        errors.append(
            "controller lease metadata path leaks OSD payload: "
            + forbidden_payload
        )

if "actorId" in api[api.find("std::string controllerLeaseJson("):
                    api.find("std::string sessionJson(")]:
    errors.append("public controller lease JSON must not expose actorId")
if "clientInstanceId" in api[api.find("std::string controllerLeaseJson("):
                              api.find("std::string sessionJson(")]:
    errors.append(
        "public controller lease JSON must not expose clientInstanceId"
    )

if errors:
    for error in errors:
        print("ERROR:", error)
    raise SystemExit(1)

print("Phase-68.F OSD controller-lease guard passed")
